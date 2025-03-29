#include "includes.h"
#include "defines.h"
#include "globals.h"
#include "forwards.h"


//used in erase config to set certificate of mqtt server
extern const uint8_t cert_start[]           asm("_binary_cert_pem_start");
extern const uint8_t cert_end[]             asm("_binary_cert_pem_end");
extern void save_inst_msg(char *mid, int bpk,int kwhstart,char *who);

int aes_encrypt(const char* src, size_t son, char *dst,const char *cualKey)
{
	int theSize=son;
	int rem= theSize % 16;
	theSize+=16-rem;			//round to next 16 for AES
	bzero(iv,sizeof(iv));

	char *donde=(char*)calloc(theSize,1);
	if (!donde)
	{
		return ESP_FAIL;
	}
	memcpy(donde,src,son);	

	if(esp_aes_setkey( &actx, (const unsigned char*)cualKey, 256 )==0)
		if(esp_aes_crypt_cbc( &actx, ESP_AES_ENCRYPT, theSize, (unsigned char*)iv, (const unsigned char*)donde, ( unsigned char*)dst )!=0)
        {
            free(donde);
            return ESP_FAIL;
        }

	free(donde);
	return theSize;
}

int aes_decrypt(const char* src, size_t son, char *dst,const unsigned char *cualKey)
{
	bzero(dst,son);
	bzero(iv,sizeof(iv));
	if(esp_aes_setkey( &actx, (const unsigned char*)cualKey, 256 )!=0)
		return ESP_FAIL;
	else
	{
		if(esp_aes_crypt_cbc( &actx, ESP_AES_DECRYPT, son,(unsigned char*) iv, ( const unsigned char*)src, (unsigned char*)dst )!=0)
			return ESP_FAIL;
		
	}
	return son;
}


void delay(uint32_t cuanto)
{
    vTaskDelay(pdMS_TO_TICKS(cuanto));
}

uint32_t xmillis()
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

uint32_t xmillisFromISR()
{
	return pdTICKS_TO_MS(xTaskGetTickCountFromISR());
}

static int findInternalCmds(const char * cual)
{
	for (int a=0;a<MAXINTCMDS;a++)
	{
		if(strcmp(internal_cmds[a],cual)==0)
			return a;
	}
	return ESP_FAIL;
}

void blinkRoot(void *pArg)
{
    while(1)
    {
        gpio_set_level((gpio_num_t)WIFILED,1);
        delay(400);
        gpio_set_level((gpio_num_t)WIFILED,0);
        delay(400);
    }
} 

void blinkConf(void *pArg)
{
    while(1)
    {
        gpio_set_level((gpio_num_t)WIFILED,1);
        gpio_set_level((gpio_num_t)BEATPIN,0);
        delay(800);
        gpio_set_level((gpio_num_t)WIFILED,0);
        gpio_set_level((gpio_num_t)BEATPIN,1);
        delay(800);
    }
} 

int get_routing_table()
{
    portMUX_TYPE    xTimerLock = portMUX_INITIALIZER_UNLOCKED;
    int             err;

    //some checks
    //new round of metrics, in case not already done, erase ram... shouldnt happend
    if(xSemaphoreTake(tableSem, portMAX_DELAY/  portTICK_PERIOD_MS))		
    {  
        for (int a=0;a<masterNode.existing_nodes;a++)
        {
            if(masterNode.theTable.thedata[a])
            {
                ESP_LOGE(MESH_TAG,"Free RAM in get route %d\n",a);
                free(masterNode.theTable.thedata[a]);
                masterNode.theTable.thedata[a]=NULL;
            }
        }

        bzero(&masterNode,sizeof(masterNode));  //always zero this stuff
        taskENTER_CRITICAL( &xTimerLock );
        err=esp_mesh_get_routing_table((mesh_addr_t *) &masterNode.theTable.big_table,MAXNODES * 6, &masterNode.existing_nodes);
        taskEXIT_CRITICAL( &xTimerLock );

        counting_nodes=masterNode.existing_nodes;  //copy for counting purposes

        time_t  now;
        theMeter.setStatsLastNodeCount(masterNode.existing_nodes);
        time(&now);
        theMeter.setStatsLastCountTS(now);
        xSemaphoreGive(tableSem);
        if(err)
            return ESP_FAIL;
        else
            return ESP_OK;
    }   // will only get here if we set a time value to the semaphore take
    return ESP_FAIL;
}

int root_delete_routing_table()
{
    int err;
    //some checks
    if(xSemaphoreTake(tableSem, portMAX_DELAY/  portTICK_PERIOD_MS))		
    {  
        for (int a=0;a<masterNode.existing_nodes;a++)
        {
            if(masterNode.theTable.thedata[a])
            {
                free(masterNode.theTable.thedata[a]);       //free ram
                masterNode.theTable.thedata[a]=NULL;
            }
        }
        // bzero(&masterNode,sizeof(masterNode));             
        counting_nodes=0;                                   
        xSemaphoreGive(tableSem);
        return ESP_OK;
    }
    return ESP_FAIL;
}

//ladata is  meshunion_t * myNode
int root_load_routing_table_mac(mesh_addr_t *who,void *ladata)
{
    int err;
    meshunion_t* aNode=(meshunion_t*)ladata;
    char midl[13];


   
    if(xSemaphoreTake(tableSem, portMAX_DELAY/  portTICK_PERIOD_MS))		
    {  
        for (int a=0;a<masterNode.existing_nodes;a++)
        {
            if (MAC_ADDR_EQUAL(masterNode.theTable.big_table[a].addr, who->addr)) 
            {
                if(masterNode.theTable.thedata[a])
                    free(masterNode.theTable.thedata[a]);           //erase old data
                masterNode.theTable.thedata[a]=ladata;              //new pointer
                // printf("Add meter [%s]\n",aNode->nodedata.metersData.meter_id);
                strcpy(masterNode.theTable.meterName[a],aNode->nodedata.metersData.meter_id);

                xSemaphoreGive(tableSem);
                return ESP_OK;
            }
        }
        xSemaphoreGive(tableSem);
        ESP_LOGE(MESH_TAG,"Did not find mac addr " MACSTR ,who->addr);
        return ESP_FAIL;
    }
}

esp_err_t send_confirmation_message(char *mid, int lstate)
{
    int                 err;
    mesh_data_t         datas;
    cJSON               *root;
    mqttSender_t        meshMsg;


        root=cJSON_CreateObject();
        if(root)
        {
            cJSON_AddStringToObject(root,"cmd",internal_cmds[CONFIRMLOCK]);
            cJSON_AddStringToObject(root,"mid",mid);
            cJSON_AddNumberToObject(root,"state",lstate);

            char *intmsg=cJSON_PrintUnformatted(root);
            if(intmsg)
            {  
                meshMsg.msg=intmsg;
                meshMsg.lenMsg=strlen(intmsg);
                meshMsg.queue=NULL;
                if(xQueueSend(meshQueue,&meshMsg,0)!=pdTRUE)      //will free todo 
                {
                    ESP_LOGE(MESH_TAG,"Error queueing confirm msg");
                    if(meshMsg.msg)
                        free(meshMsg.msg);  //due to failure
                    return ESP_FAIL;
                }
                ESP_LOGI(MESH_TAG,"Confirmation sent for Meter [%s]",mid);
            }
            else
            {
                ESP_LOGE(MESH_TAG,"No RAM Confirm");
                cJSON_Delete(root); 
                return ESP_FAIL;
            }
            cJSON_Delete(root); 
            return ESP_OK;
        }
        else
            ESP_LOGE(MESH_TAG,"No Root Confirm");

        return ESP_FAIL;
}

esp_err_t send_metrics_message(char *mid)
{
    int                 err;
    mesh_data_t         datas;
    cJSON               *root;
    mqttSender_t        meshMsg;
    char                buff[30];

        root=cJSON_CreateObject();
        if(root)
        {
            cJSON_AddStringToObject(root,"cmd",internal_cmds[METRICRESP]);
            cJSON_AddStringToObject(root,"mid",mid);
            cJSON_AddNumberToObject(root,"kwh",theMeter.getLkwh());
            cJSON_AddNumberToObject(root,"beat",theMeter.getBeats());
            cJSON_AddNumberToObject(root,"maxamp",theMeter.getMaxamp());
            cJSON_AddNumberToObject(root,"minamp",theMeter.getMinamp());
            time_t lastup=theMeter.getLastUpdate();
            strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&lastup));
            cJSON_AddStringToObject(root,"update",buff);

            char *intmsg=cJSON_PrintUnformatted(root);
            if(intmsg)
            {  
                meshMsg.msg=intmsg;
                meshMsg.lenMsg=strlen(intmsg);
                meshMsg.queue=NULL;
                if(xQueueSend(meshQueue,&meshMsg,0)!=pdTRUE)      //will free todo 
                {
                    ESP_LOGE(MESH_TAG,"Error queueing confirm msg");
                    if(meshMsg.msg)
                        free(meshMsg.msg);  //due to failure
                    return ESP_FAIL;
                }
                ESP_LOGI(MESH_TAG,"Confirmation sent for Meter [%s]",mid);
            }
            else
            {
                ESP_LOGE(MESH_TAG,"No RAM Metrics");
                cJSON_Delete(root); 
                return ESP_FAIL;
            }
            cJSON_Delete(root); 
            return ESP_OK;
        }
        else
            ESP_LOGE(MESH_TAG,"No Root Metrics");

        return ESP_FAIL;
}

int  turn_display(char *cmd,char *mid)
{
    cJSON               *cualm,*ltime,*elcmd;
    if(!gdispf)
        return ESP_FAIL;
    elcmd=cJSON_Parse(cmd);
    if(elcmd)
    {
        cualm= 		cJSON_GetObjectItem(elcmd,"mid");
        if(cualm)
        {
            ltime= 		cJSON_GetObjectItem(elcmd,"time");
            if(!ltime)
            {
                ESP_LOGE(MESH_TAG,"Display No state time");
                ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
                return ESP_FAIL;
            }
            int duration=ltime->valueint*1000;

           if(strcmp(cualm->valuestring,theMeter.getMID())==0)
           {
                ESP_LOGW(MESH_TAG,"Display MId [%s me] Time [%d]",cualm->valuestring,ltime->valueint);
                if(!showHandle && duration>0)     //is it not active
                {
                    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
                    xTaskCreate(&showData,"sdata",1024*3,NULL, 5, &showHandle); 
                    dispTimer=xTimerCreate("DispT",pdMS_TO_TICKS(duration),pdFALSE,NULL, []( TimerHandle_t xTimer)
                    { 				
                        vTaskDelete(showHandle);
                        showHandle=NULL;
                        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));

                    });   
                    xTimerStart(dispTimer,0);
                }
                else
                {
                    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
                    if(showHandle)
                        vTaskDelete(showHandle);
                    showHandle=NULL;
                    if(dispTimer)
                        xTimerStop(dispTimer,0);

                }
           }
           else
            ESP_LOGI(MESH_TAG,"It wasnt me Display!");
        }
        else 
        {
            ESP_LOGE(MESH_TAG,"Display Cmd no MID\n");
            return ESP_FAIL;
        }
        cJSON_free(elcmd);
    }
    else 
    {
        ESP_LOGE(MESH_TAG,"NO CMD argument passed. Turn Display error\n");
    }
    return ESP_OK;
}

int  check_my_meter(char *cmd,char *mid)
{
    cJSON               *cualm,*lstate,*elcmd;

    elcmd=cJSON_Parse(cmd);
    if(elcmd)
    {
        cualm= 		cJSON_GetObjectItem(elcmd,"mid");
        if(cualm)
        {
            strcpy(mid,cualm->valuestring);
            lstate= 		cJSON_GetObjectItem(elcmd,"state");
            if(!lstate)
            {
                ESP_LOGE(MESH_TAG,"Lock No state lock");
                return ESP_FAIL;
            }
           if(strcmp(cualm->valuestring,theMeter.getMID())==0)
           {
                ESP_LOGW(MESH_TAG,"MId [%s me] Lock [%s]",cualm->valuestring,lstate->valueint?"Disc":"Conn");
            //confirm lock message
            theMeter.turn_onoff_meter(lstate->valueint,true);
            send_confirmation_message(cualm->valuestring,lstate->valueint);
            char *tmp=(char*)calloc(100,1);
            if(tmp)
            {
                sprintf(tmp,"MId [%s me] Lock [%s]",cualm->valuestring,lstate->valueint?"Disc":"Conn");
                writeLog(tmp);
                free(tmp);
            }
           }
           else
            ESP_LOGI(MESH_TAG,"It wasnt me!");
        }
        else 
        {
            ESP_LOGE(MESH_TAG,"Lock Cmd no MID\n");
            return ESP_FAIL;
        }
        cJSON_free(elcmd);
    }
    else 
    {
        ESP_LOGE(MESH_TAG,"NO CMD argument passed. Internal error\n");
    }
    return ESP_OK;
}
int  check_my_metrics(char *cmd,char *mid)
{
    cJSON               *cualm,*lstate,*elcmd;

    elcmd=cJSON_Parse(cmd);
    if(elcmd)
    {
        cualm= 		cJSON_GetObjectItem(elcmd,"mid");
        if(cualm)
        {
            strcpy(mid,cualm->valuestring);
            if(strcmp(cualm->valuestring,theMeter.getMID())==0)
            {
                ESP_LOGW(MESH_TAG,"MId [%s me] Metrics",cualm->valuestring);
            //send metrics message
                send_metrics_message(cualm->valuestring);
                // char *tmp=(char*)calloc(100,1);
                // if(tmp)
                // {
                //     sprintf(tmp,"MId [%s me] Lock [%s]",cualm->valuestring,lstate->valueint?"Disc":"Conn");
                //     writeLog(tmp);
                //     free(tmp);
                // }
            }
           else
            ESP_LOGI(MESH_TAG,"It wasnt me metrics!");
        }
        else 
        {
            ESP_LOGE(MESH_TAG,"Metrics Cmd no MID\n");
            return ESP_FAIL;
        }
        cJSON_free(elcmd);
    }
    else 
    {
        ESP_LOGE(MESH_TAG,"NO CMD argument passed. Metrics Internal error\n");
    }
    return ESP_OK;
}

int  update_my_meter(char *cmd)     
{
    cJSON               *cualm,*bpk,*ks,*k,*elcmd;

    elcmd=cJSON_Parse(cmd);
    if(elcmd)
    {
        cualm= 		cJSON_GetObjectItem(elcmd,"mid");
        bpk= 		cJSON_GetObjectItem(elcmd,"bpk");
        ks= 		cJSON_GetObjectItem(elcmd,"ks");
        k= 		    cJSON_GetObjectItem(elcmd,"k");
        if(cualm && bpk && k && ks)
        {
            if(strcmp(theMeter.getMID(),cualm->valuestring)==0)
            {
                if(k->valueint<ks->valueint)
                {
                    ESP_LOGW(MESH_TAG,"Update kwh [%d} < Kwstart [%d}",k->valueint,ks->valueint);
                    cJSON_free(elcmd);
                    return  ESP_FAIL;
                }
                char *tmp=(char*)calloc(200,1);
                if(tmp)
                {
                    sprintf(tmp,"MId [%s me] Update Bpk:[%d->%d} kWh:[%d->%d] kWhStart:[%d->%d]",cualm->valuestring,theMeter.getBPK(),bpk->valueint,
                                theMeter.getLkwh(),k->valueint,theMeter.getKstart(),ks->valueint);
                    writeLog(tmp);
                    free(tmp);
                }
                theMeter.eraseMeter();           //erase all metrics given new ks and k
                theMeter.setBPK(bpk->valueint);
                theMeter.setLkwh(k->valueint);
                theMeter.setKstart(ks->valueint);
                theMeter.saveMeter();

           }
           else
            ESP_LOGI(MESH_TAG,"Update It wasnt me!");
        }
        else 
        {
            ESP_LOGE(MESH_TAG,"Update Cmd missing required param\n");
            return ESP_FAIL;
        }
        cJSON_free(elcmd);
    }
    else 
    {
        ESP_LOGE(MESH_TAG,"NO CMD argument passed. Update Internal error\n");
    }
    return ESP_OK;
}

// internal Mesh network message, we use cjson since no big limit
// used to give to all connecting nodes certain "global" system parameter
// Date, last known ssid and passw, time slot, mqtt params and Application location params like prov, canton,etc
// Only Root sends this message to connecting Child

esp_err_t root_send_data_to_node(mesh_addr_t thismac)
{
    int                 err;
    wifi_config_t       configsta;
    mesh_data_t         data;
    char                *topic;
    time_t              now;
    cJSON               *root;
  

    time(&now);
    err=esp_wifi_get_config( WIFI_IF_STA,&configsta);      // get station ssid and password
    if(!err)
    {
        root=cJSON_CreateObject();
        if(root)
        {
            cJSON_AddStringToObject(root,"cmd",internal_cmds[BOOTRESP]);
            cJSON_AddStringToObject(root,"ssid",(char*)configsta.sta.ssid);
            cJSON_AddStringToObject(root,"psw",(char*)configsta.sta.password);
            cJSON_AddNumberToObject(root,"time",(uint32_t)now);
            cJSON_AddNumberToObject(root,"slot",theConf.mqttSlots);
            cJSON_AddNumberToObject(root,"prov",theConf.provincia);
            cJSON_AddNumberToObject(root,"parro",theConf.parroquia);
            cJSON_AddNumberToObject(root,"cant",theConf.canton);
            cJSON_AddNumberToObject(root,"nodeid",theConf.controllerid);
            cJSON_AddStringToObject(root,"mqtts",theConf.mqttServer);
            cJSON_AddStringToObject(root,"mqttu",theConf.mqttUser);
            cJSON_AddStringToObject(root,"mqttp",theConf.mqttPass);

            char *intmsg=cJSON_PrintUnformatted(root);
            if(intmsg)
            {  
                data.data   =(uint8_t*)intmsg;
                data.size   =strlen(intmsg);
                data.proto  = MESH_PROTO_BIN;
                data.tos    = MESH_TOS_P2P;

                err= esp_mesh_send( &thismac, &data, MESH_DATA_P2P, NULL, 0);   //just this node
                if(err)
                    printf("Broadcast failed %x\n",err);
                free(intmsg);
            }
            cJSON_Delete(root); 
            return ESP_OK;
        }
        else
            return ESP_FAIL;
    }
    else
        return ESP_FAIL;
}

int root_sendNodeACK(void *parg)
{
    mesh_data_t data;

    mesh_addr_t *from=(mesh_addr_t*)parg;
    // printf("Send Node Install confirmation sent to "MACSTR"\n",MAC2STR(from->addr));
    char *mensaje=(char*)calloc(1,100);
    if(mensaje)
    {
        strcpy(mensaje,"{\"cmd\":\"installconf\"}");      //cJSON is long /elaborate for this simple message
        data.proto = MESH_PROTO_BIN;
        data.tos = MESH_TOS_P2P;
        data.data=(uint8_t*)mensaje;
        data.size=strlen(mensaje);
        int err = esp_mesh_send(from, &data, MESH_DATA_P2P, NULL, 0);
        free(mensaje);
    }
    return 0;
}

esp_err_t root_send_confirmation_central(char *msg,uint16_t size,char *cualQ)
{
    mqttSender_t        mqttMsg;
        char *confmsg=(char*)calloc(1,size);
        // printf("SentoCentral [%s]/%d to Queue [%s]\n",msg,size,cualQ);
        memcpy(confmsg,msg,size);
        mqttMsg.queue=cualQ;
        mqttMsg.msg=confmsg;                                    // freed by mqtt sender
        mqttMsg.lenMsg=size;
        mqttMsg.code=NULL;
        mqttMsg.param=0;
        if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)       
        {
            ESP_LOGE(MESH_TAG,"Error queueing confirm msg %s",cualQ);
            if(mqttMsg.msg)
                free(mqttMsg.msg);  //due to failure
            return ESP_FAIL;
        }
        return ESP_OK;
}

esp_err_t root_send_confirmation_central_cb(char *msg,char *cualQ,mesh_addr_t *from)
{
    mqttSender_t        mqttMsg;
    mesh_addr_t         *localcopy;

        localcopy=(mesh_addr_t*)calloc(1,sizeof(mesh_addr_t));
        memcpy(localcopy,from,sizeof(mesh_addr_t));
        // printf("SentoCentral [%s]/%d to Queue [%s]\n",msg,strlen(msg),cualQ);
        mqttMsg.queue=cualQ;
        mqttMsg.msg=msg;                                    // freed by mqtt sender
        mqttMsg.lenMsg=strlen(msg);
        mqttMsg.code=root_sendNodeACK;
        mqttMsg.param=(uint32_t*)localcopy;                   //the beaty of C++ do whatever u want it will leak...now freed also when sent
        if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)       
        {
            ESP_LOGE(MESH_TAG,"Error queueing confirm msg %s",cualQ);
            if(mqttMsg.msg)
                free(mqttMsg.msg);  //due to failure
            if(mqttMsg.param)
                free(mqttMsg.param);
            return ESP_FAIL;
        }
        return ESP_OK;
}

esp_err_t root_send_collected_nodes(uint32_t cuantos)        //root only
{
    mqttSender_t        mqttMsg;
    meshunion_t*        aNode;

    theMeter.setStatsLastMeterCount(cuantos);
            // TickType_t xRemainingTime =xTimerGetExpiryTime( sendMeterTimer ) - xTaskGetTickCount();
            // ESP_LOGI(MESH_TAG,"Collect Data time %dms",10000-pdTICKS_TO_MS(xRemainingTime));
 
            //build mqtt message
            //since its binary and can change the number of nodes, first uint32 of message is the number of Nodes in this message
           
            uint32_t hp=esp_get_free_heap_size();
            ESP_LOGW(MESH_TAG,"Collected NODES heap %d son %d",hp,cuantos);

            int totalSize=(cuantos*sizeof(meshunion_t))+sizeof(uint32_t)+sizeof(uint32_t); // node count and heap for debugging. Cuantos used alos in teimout call to send just CUANTOS nodes
            uint8_t *todo=(uint8_t*)calloc(totalSize,1);
            uint8_t *copystart=todo;                                                // variable todo will be increased so we need original start
            memcpy(todo,&cuantos,sizeof(uint32_t));                                  //node count
            todo+=sizeof(cuantos);                                                  //move ptr 4 bytes
            memcpy(todo,&hp,sizeof(uint32_t));                                            //heap count
            todo+=sizeof(hp);                                                       //move ptr 4 bytes
            lastKnowCount=cuantos;

            for (int a=0;a<cuantos;a++)                                             //data is stored in the MasterNode Structure
            {
                if(masterNode.theTable.thedata[a])                                  //if data present
                {
                    memcpy(todo,masterNode.theTable.thedata[a],sizeof(meshunion_t));
                    todo+=sizeof(meshunion_t);                                                              //increase pointer by meshunion size
                }
                else 
                    printf("Error null data on node %d\n",a);
            }

            if(root_delete_routing_table()!=ESP_OK)
            {
                ESP_LOGE(MESH_TAG,"Error deleting Routing table");
                free(copystart);
                return ESP_FAIL;

            }

            //send mqtt msg. Use mqtt queue
            mqttMsg.queue=                      infoQueue;
            mqttMsg.msg=                        (char*)copystart;
            mqttMsg.lenMsg=                     totalSize;
            mqttMsg.code=                       NULL;
            mqttMsg.param=                      NULL;
            if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)      //will free todo 
            {
                ESP_LOGE(MESH_TAG,"Error queueing msg");
                if(mqttMsg.msg)
                    free(mqttMsg.msg);  //due to failure
                return ESP_FAIL;
            }
            else
            {
                    //must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
                xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// Send everything now !!!!!
            }

    return ESP_OK;

}

// works with a timer in case an expected node never sends the meter data
// Gets messages sent from ohter Child Nodes with their meter reading. When last one "counting_nodes" is received send the Data to MQTT HQ
// using Mqtts segmenting capabilities 
// ladata is  meshunion_t * myNode
esp_err_t root_check_incoming_meters(mesh_addr_t *who, void* ladata)
{
    mqttSender_t        mqttMsg;
    esp_err_t           err;

    if(root_load_routing_table_mac(who,ladata)==ESP_OK)      //svae messages in our masternode table
    {
        counting_nodes--;
        if(counting_nodes==0)                           //when we reach existing node count, start sending process
        {
            //stop watchdog timer
            if( xTimerStop(sendMeterTimer, 0 ) != pdPASS )
                ESP_LOGE(MESH_TAG,"Failed to stop SendMeter Timer mqtt_published nodes");
                    
            err= root_send_collected_nodes(masterNode.existing_nodes);        //all existing nodes
            if(err)
            {
                    ESP_LOGE(MESH_TAG,"Error sending mqtt msg");
                    return ESP_FAIL;
            }
            else
                return ESP_OK;   
            
        }
        else
            return ESP_OK;
    }
    else
        return ESP_FAIL;
}

void root_timer(void *parg)
{
    mqttSender_t        mqttMsg;
    esp_err_t           err;
    
    while(true)
    {
    xEventGroupWaitBits(otherGroup,REPEAT_BIT,pdTRUE,pdFALSE,portMAX_DELAY);    //wait forever, this is the starting gun, flag will be cleared

    ESP_LOGW(MESH_TAG,"Mesh Timeout. Will send only %d nodes",masterNode.existing_nodes-counting_nodes);

    if(sendMeterf)      //recheck we are sending mode
    {
        if((masterNode.existing_nodes-counting_nodes)>0)
        {
            err= root_send_collected_nodes(masterNode.existing_nodes-counting_nodes);        //existing nodes minus pending
            if(err)
                ESP_LOGE(MESH_TAG,"Error sending meter timeout msg");
        }
        else
            sendMeterf=false;       //reset flag
    }
            if( xTimerStart(repeatTimer, 0 ) != pdPASS )
                ESP_LOGE(MESH_TAG,"Repeat Timer mqtt saender failed");
        }
}


void root_collect_meter_data(TimerHandle_t algo)
{
    int                 err;    
    mesh_data_t         data;
    char                *buf;
    struct tm           timeinfo;
    time_t              now;
    char*               broadcast;

  portMUX_TYPE xTimerLock = portMUX_INITIALIZER_UNLOCKED;

// get routing table if root
    if(esp_mesh_is_root())
    {
        if(sendMeterf )
        {
            ESP_LOGW(MESH_TAG,"Collect called without previous called finished"); //very import check, else HEAP collapse eventually
            //maybe just reset the flag and return
            // analyze this carefully its vital
            // the flag is reset by the MQtt manager meaning a message was sent
            sendMeterf=false;
            return;
        }
        // meterCount=0;           //reset meter counter of next data collection

        err=get_routing_table();        // Get our routing table, This is how many nodes we should esxpect messages from
        if(err)
            ESP_LOGE(MESH_TAG,"Could not get routing table FATAL");

        // send Broadcast  to start receiving meters from nodes 
        broadcast=(char*)calloc(50,1);
        if(!broadcast)
        {
            ESP_LOGE(MESH_TAG,"Could not calloc collect meter FATAL");
            return;
        }
        sendMeterf=true;            //now in sendmeter mode so start timer on this message to nodes
   
        if( xTimerStart(sendMeterTimer, 0 ) != pdPASS )
            ESP_LOGE(MESH_TAG,"SendMeter Timer failed");
        strcpy(broadcast,"{\"cmd\":\"sendmetrics\"}");      //cJSON is long /elaborate for this simple message
        data.data   =(uint8_t*)broadcast;
        data.size   = strlen(broadcast);
        data.proto  = MESH_PROTO_BIN;
        data.tos    = MESH_TOS_P2P;
        //send a Broadcast Message to all nodes to send their data
        err= esp_mesh_send( &GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);         //broadcast msg to mesh, must be freed by us
        if(err)
        {
            ESP_LOGE(MESH_TAG,"Broadcast failed. Now message in this slots %x",err);
            root_delete_routing_table();    //????? why
            free(broadcast); 
            sendMeterf=false;
            return; 
        }
        free(broadcast);    
    }
}

//TODO check if cjText is the lojng verison not parajson one
void set_sta_cmd(char *cjText)      //message from Root giving stations ids and passwords 
{
    int err;
    wifi_config_t       configsta;
    struct timeval      now;

    cJSON *elcmd=cJSON_Parse(cjText);
    if(!elcmd)
    {
        ESP_LOGE(MESH_TAG,"Invalid Station Text received [%s]",cjText);
        return;
    }
    err=esp_wifi_get_config( WIFI_IF_STA,&configsta);      // get station ssid and password and others
    if(!err)
    {
        cJSON *ssid= 		cJSON_GetObjectItem(elcmd,"ssid");
        cJSON *pswd= 		cJSON_GetObjectItem(elcmd,"psw");
        cJSON *time= 		cJSON_GetObjectItem(elcmd,"time");
        cJSON *slot= 		cJSON_GetObjectItem(elcmd,"slot");
        cJSON *prov= 		cJSON_GetObjectItem(elcmd,"prov");
        cJSON *parro= 		cJSON_GetObjectItem(elcmd,"parro");
        cJSON *cant= 		cJSON_GetObjectItem(elcmd,"cant");
        cJSON *nodeid= 		cJSON_GetObjectItem(elcmd,"nodeid");
        cJSON *mqttserver=	cJSON_GetObjectItem(elcmd,"mqtts");
        cJSON *mqttuser= 	cJSON_GetObjectItem(elcmd,"mqttu");
        cJSON *mqttpass=	cJSON_GetObjectItem(elcmd,"mqttp");
        if(ssid && pswd)
        {
            ESP_LOGI(MESH_TAG,"Set Local STA Non Root ssid [%s] passw[%s]",ssid->valuestring,pswd->valuestring);
            memcpy(&configsta.sta.ssid,ssid->valuestring,strlen(ssid->valuestring));
            memcpy(&configsta.sta.password,pswd->valuestring,strlen(pswd->valuestring));                        //set ssid and password of internal NVS configuration
            err=esp_wifi_set_config( WIFI_IF_STA,&configsta);                                                   // save new ssid and password
            if(err)
                ESP_LOGE(MESH_TAG,"Failed to save new ssid %x",err);
            else
            {
                strcpy(theConf.thepass,pswd->valuestring);
                strcpy(theConf.thessid,ssid->valuestring);
            }
        }
        else
            ESP_LOGE(MESH_TAG,"Update SSID without ssid or pswd");

        //set time
        setenv("TZ", LOCALTIME, 1);
        tzset();
        now.tv_sec=time->valueint;
        now.tv_usec=0;
        settimeofday(&now, NULL);

        //set location parameters
        theConf.mqttSlots=slot->valueint;
        theConf.provincia=prov->valueint;
        theConf.parroquia=parro->valueint;
        theConf.canton=cant->valueint;
        theConf.controllerid=nodeid->valueint;   
        strcpy(theConf.mqttServer,mqttserver->valuestring);
        strcpy(theConf.mqttUser,mqttuser->valuestring);
        strcpy(theConf.mqttPass,mqttpass->valuestring);

        write_to_flash();      

        //cmd and info queue names derived form the Config so do it now
    sprintf(cmdQueue,"%s/%d/cmd",QUEUE,theConf.controllerid);
    sprintf(infoQueue,"%s/%d/info",QUEUE,theConf.controllerid);
    sprintf(emergencyQueue,"%s/911",QUEUE,theConf.controllerid);
    sprintf(cmdBroadcast,"%s/broadcast",QUEUE);
    sprintf(discoQueue,"%s/disco",QUEUE);
    sprintf(installQueue,"%s/install",QUEUE);
    }
    else
        ESP_LOGE(MESH_TAG,"Could not get STA config update ssid %x", err);       
    cJSON_Delete(elcmd);
}


esp_err_t send_datos_to_root()
{
    int                 err;
    mesh_data_t         datas;

// if (theConf.meterconf!=2)
//     return ESP_FAIL;                //will not send until configured

    meshunion_t * myNode=sendData(true);
    if(!myNode)
    {
        printf("Error send datos root msg\n");
        return -2;
    }

        datas.data      =(uint8_t*)myNode;
        datas.size      = sizeof(meshunion_t);
        datas.proto     = MESH_PROTO_BIN;
        datas.tos       = MESH_TOS_P2P;
        err= esp_mesh_send( NULL, &datas, MESH_DATA_P2P, NULL, 1); 
        if(err)
            ESP_LOGE(MESH_TAG,"Send Meters failed %x",err);
        // esp_mesh_flush_upstream_packets();  //force sending
        free(myNode);           //important
        return ESP_OK;
}

void format_my_meter(cJSON *cmd)
{
    time_t  now;
    cJSON *passw= 		cJSON_GetObjectItem(cmd,"passw");
    cJSON *mid= 		cJSON_GetObjectItem(cmd,"mid");
    cJSON *reconf= 		cJSON_GetObjectItem(cmd,"erase");
        if(mid)
        {
            if (strcmp(mid->valuestring,theMeter.getMID())==0)
            {
                    theMeter.deinit();
                    theMeter.format();
                    time(&now);
                    theMeter.writeCreationDate(now);
                if(reconf)
                {
                    printf("Erase %s\n",reconf->valuestring);
                    if(strcmp(reconf->valuestring,"Y")==0)
                        erase_config(); 
                }

                char *tmp=(char*)calloc(100,1);
                if(tmp)
                {
                    sprintf(tmp,"Format meter [%s] FRAM Command executed",theMeter.getMID());
                    writeLog(tmp);
                    fclose(myFile);
                    free(tmp);
                }                     
                    ESP_LOGW(MESH_TAG,"Format MID [%s] sent to [%s] Fram and Erase configuration. Virgin chip",theMeter.getMID(),mid->valuestring);
                    delay(1000);
                    esp_restart();                
                }
        }

}

void erase_my_meter(cJSON *cmd)
{
    time_t  now;
    cJSON *mid= 		cJSON_GetObjectItem(cmd,"mid");
        if(mid)
        {
            if (strcmp(mid->valuestring,theMeter.getMID())==0)
            {

                theMeter.eraseMeter();//erase kws/kwh/and other fields and save the same MID

                char *tmp=(char*)calloc(100,1);
                if(tmp)
                {
                    sprintf(tmp,"Erase meter [%s] Command executed",theMeter.getMID());
                    writeLog(tmp);
                    fclose(myFile);
                    free(tmp);
                }                               
            }
        }

}

//here we received all mesh traffic data
void static mesh_manager(mesh_addr_t *from, mesh_data_t *data)
{
    mqttSender_t        mqttMsg;
    cJSON 	            *elcmd;
    char                *message,*temp,*msg2;
    static char         cualMID[20];
    mesh_data_t         datas;
    meshunion_t *       aNode;
    esp_err_t           err;
    locker_t            lock;
    char                *tmp, *msg;
//msgs from the MESH can be json or binary, check it out

    mqttMsg.queue=infoQueue;            //set as defualt Queue for mqtt
    mqttMsg.code=NULL;
    mqttMsg.param=NULL;
    aNode=(meshunion_t *)calloc(data->size,1);
    if(!aNode)
    {
        ESP_LOGE(MESH_TAG,"mesh RECV Failed calloc");
        return;
    }

    if(data->size==0)
    {
        ESP_LOGE(MESH_TAG,"Meshmgr got 0 len");
        free(aNode);
        return;
    }
        // make a local copy, data is static so cannot be shared/passed
    memcpy(aNode,(char*)data->data,data->size);

    elcmd=cJSON_Parse(aNode->parajson);                             
    if(!elcmd)                                                      //its binary meter data msg, not internal commands
    {
        if(esp_mesh_is_root())                                      //only ROOT is central manager. ONE connection to MQTT
        {
            if(root_check_incoming_meters(from,aNode)!=ESP_OK)      //saves data into a working table unless error
            {
                ESP_LOGE(MESH_TAG,"Check Incoming Error");
                free(aNode);
            }
    // do not free elcmd since it was not created
    // DO NOT Free aNode its used to get the sent data by Load_table
            return;
        }
    }
//reparse in case data len is greater than parajson
    cJSON_Delete(elcmd);        ///MUST since we are going to replicate parse again below
    elcmd=cJSON_Parse((char*)data->data);  
    if(!elcmd)
    {
        ESP_LOGI(MESH_TAG,"No elcmd");
        return;
    }
    cJSON *cualm= 		cJSON_GetObjectItem(elcmd,"cmd");
    if(cualm)
    {
        ESP_LOGI(MESH_TAG,"find internal cmd %s",cualm->valuestring);
        int cualf=findInternalCmds(cualm->valuestring);
        if(cualf>=0)
        {
            switch (cualf)
            {
                // ESP_LOGI(MESH_TAG,"Found %d",cualf);
                case EMERGENCY: //911 call
                        ESP_LOGI(MESH_TAG,"Mesh 911 call");
                        mqttMsg.queue=emergencyQueue;   // fall thru  
                        message=(char*)calloc(data->size,1);   //will be freed by mqttsender at delivery confirmation
                        memcpy(message,data->data,data->size);
                        //need to free msg from fram find him
                        mqttMsg.msg=message;
                        mqttMsg.lenMsg=data->size;
                        mqttMsg.code=NULL;
                        mqttMsg.param=NULL; 
                        break;                             
                case BOOTRESP: // a msg from Root giving the current ssid/pswd bootresp
                        ESP_LOGI(MESH_TAG,"Mesh Boot response");
                        if(!esp_mesh_is_root()) //only non ROOT 
                            set_sta_cmd(aNode->parajson);
                        free(aNode); 
                        cJSON_Delete(elcmd);
                        return;
                case SENDMETRICS:// host requires Node sends its meter data 
                        ESP_LOGI(MESH_TAG,"Send Meters cmd from Host");
                        err=send_datos_to_root();
                        if(err)
                            ESP_LOGE(MESH_TAG,"Error send Meters %d",err);
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;
                case REINSTALL:// meter data from nodes
                    {
                        ESP_LOGI(MESH_TAG,"Mesh Reinstall");
                        theConf.meterconf=1;
                        save_inst_msg(theMeter.getMID(),theMeter.getBPK(),theMeter.getKstart(),"system");
                        cJSON_Delete(elcmd);
                        esp_restart();
                        return;
                    }  
                case LOCKMETER:// lock/unlock a meter
                        ESP_LOGI(MESH_TAG,"Mesh Lock Cmd");
                        bzero(cualMID,strlen(cualMID));
                        check_my_meter((char*)data->data,cualMID);
                        if(esp_mesh_is_root())
                        {
                            vTimerSetTimerID( confirmTimer, &cualMID );
                            xTimerStart(confirmTimer,0);
                        }
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;
                case UPDATEMETER:// update important settings meter
                        ESP_LOGI(MESH_TAG,"Mesh Update Mesh Cmd");
                        update_my_meter((char*)data->data);
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;
                case CONFIRMLOCK:// lock confirm
                        if(esp_mesh_is_root())
                        {
                            ESP_LOGI(MESH_TAG,"Mesh Lock Confirm");
                            xTimerStop(confirmTimer,0);
                            root_send_confirmation_central((char*)data->data,data->size,discoQueue);
                            free(aNode);
                            cJSON_Delete(elcmd);
                            return;
                        }
                        break;    
                case INSTALLATION:
                        ESP_LOGI(MESH_TAG,"Mesh Sending installation confirmation");
                        msg=(char*)calloc(1,data->size+1);
                        if(msg)
                        {
                            memcpy(msg,data->data,data->size);
                            if(root_send_confirmation_central_cb(msg,installQueue,from)==ESP_FAIL)
                                ESP_LOGE(MESH_TAG,"Error sending install to Mqtt Mgr");
                        }
                        else
                        {
                            ESP_LOGE(MESH_TAG,"Error RAM Install mesh mgr");
                            free(msg);
                        }
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;
                case CONFIRMINST:// Root sending confirmation Install was sent to Main Server
                        ESP_LOGW(MESH_TAG,"Mesh Install Host Confirm");
                        theConf.meterconf=2;
                        bzero(theConf.instMsg,sizeof(theConf.instMsg));
                        theConf.instMsglen=0;
                        write_to_flash();
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;
                case FORMAT:    // format the meter
                        ESP_LOGI(MESH_TAG,"Mesh Format cmd");
                        format_my_meter(elcmd);
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;
                case ERASEMETRICS:    // erase metrics
                        ESP_LOGI(MESH_TAG,"Mesh Erase Metrics cmd");
                        erase_my_meter(elcmd);
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;
                case MQTTMETRICS:
                        ESP_LOGI(MESH_TAG,"Mesh Metrics requested cmd");
                        check_my_metrics((char*)data->data,cualMID);
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;     
                case METRICRESP:
                        ESP_LOGI(MESH_TAG,"Mesh Metrics respond cmd");
                        msg=(char*)calloc(1,data->size+1);
                        if(msg)
                        {
                            memcpy(msg,data->data,data->size);
                            if(root_send_confirmation_central_cb(msg,installQueue,from)==ESP_FAIL)
                                ESP_LOGE(MESH_TAG,"Error sending meterics response to Mqtt Mgr");
                        }
                        else
                        {
                            ESP_LOGE(MESH_TAG,"Error RAM Response metrics mesh mgr");
                            free(msg);
                        }
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;     
                case SHOWDISPLAY:
                        ESP_LOGI(MESH_TAG,"Mesh Display cmd");
                        turn_display((char*)data->data,cualMID);
                        free(aNode);
                        cJSON_Delete(elcmd);
                        return;     
                default:
                        ESP_LOGE(MESH_TAG,"Mesh Internal not found %s",cualm->valuestring);
                        cJSON_Delete(elcmd);
                        return;
            }
        }
        else
        {
            ESP_LOGW(MESH_TAG,"Mesh cmd not found");
            cJSON_Delete(elcmd);
            return;
        }
    }
    else
        {
            ESP_LOGI(MESH_TAG,"Not binary and not cjson [%s]",data->data);
            esp_log_buffer_hex(MESH_TAG,data->data,60);
        }
// binary data of nodes
//TODO need to review this I dont think its used or should be used
    
    ESP_LOGI(MESH_TAG,"Processing other messages"); //use by Emergency services
    free(aNode);
    cJSON_Delete(elcmd);
    if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)      //will free message malloc
        {
            ESP_LOGE(MESH_TAG,"Error queueing msg emergency");
            if(mqttMsg.msg)
                free(mqttMsg.msg);  //due to failure
        }
    else
            //must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
        xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// Send everything now !!!!!
}

static void read_flash()
{
	esp_err_t q ;
	size_t largo;

	if(xSemaphoreTake(flashSem, portMAX_DELAY/  portTICK_PERIOD_MS))
	{
		q = nvs_open("config", NVS_READONLY, &nvshandle);
		if(q!=ESP_OK)
		{
			ESP_LOGE(MESH_TAG,"Error opening NVS Read File %x",q);
			xSemaphoreGive(flashSem);
			return;
		}

		largo=sizeof(theConf);
		q=nvs_get_blob(nvshandle,"sysconf",(void*)&theConf,&largo);

		if (q !=ESP_OK)
			ESP_LOGE(MESH_TAG,"Error read %x largo %d aqui %d",q,largo,sizeof(theConf));
		nvs_close(nvshandle);
		xSemaphoreGive(flashSem);
	}
}

void write_to_flash() //save our configuration
{
	if(xSemaphoreTake(flashSem, portMAX_DELAY/  portTICK_PERIOD_MS))
	{
		esp_err_t q ;
		q = nvs_open("config", NVS_READWRITE, &nvshandle);
		if(q!=ESP_OK)
		{
			ESP_LOGE(MESH_TAG,"Error opening NVS File RW %x",q);
			xSemaphoreGive(flashSem);
			return;
		}
		size_t req=sizeof(theConf);
		q=nvs_set_blob(nvshandle,"sysconf",&theConf,req);
		if (q ==ESP_OK)
		{
			q = nvs_commit(nvshandle);
			if(q!=ESP_OK)
				ESP_LOGE(MESH_TAG,"Flash commit write failed %d",q);
		}
		else
			ESP_LOGE(MESH_TAG,"Fail to write flash %x",q);
		nvs_close(nvshandle);
		xSemaphoreGive(flashSem);
	}
}

void root_sntpget(void *pArg)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    timeval localtime;

    memset(&timeinfo,0,sizeof(timeinfo));
    
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) 
    {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();

        int retry = 0;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= SNTPTRY)
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        
        if(retry>=SNTPTRY)
        {
            ESP_LOGE(MESH_TAG,"SNTP failed retry count, using saved time");
            theConf.lastKnownDate=theMeter.getReservedDate();   
            localtime.tv_sec=theConf.lastKnownDate;
            localtime.tv_usec=0;                                      // no connection most likely
            settimeofday(&localtime,NULL);
            //let him fall thru and continue as if date was received
        }
        time(&now);
        setenv("TZ", LOCALTIME, 1);
        tzset();
        if(theConf.lastRebootTime==0)
            theConf.lastRebootTime=now;
        else
            theConf.downtime+=(uint32_t)now-theConf.lastRebootTime;
        theMeter.setReservedDate(now);

        ESP_LOGI(MESH_TAG,"SNTP Date %s",ctime((time_t*)&now));
        
        //start our Cycle calculations
        root_set_senddata_timer();
        write_to_flash();
    }
    vTaskDelete(NULL);

}

void root_reconnectTask(void *pArg)
{
    esp_err_t err;
    EventBits_t uxBits;
    int retry=0;

    // in case timers are active
    xTimerStop(repeatTimer, 0 );
    xTimerStop(sendMeterTimer, 0 );
    //kill MQTT Tasks
    vTaskDelete(mqttMgrHandle);
    vTaskDelete(mqttSendHandle);

    while(true)
    {
        //try to reconnect
        xEventGroupClearBits(wifi_event_group,MQTT_BIT|ERROR_BIT|DISCO_BIT);        // clear bits
        err=esp_mqtt_client_reconnect(clientCloud);
        // if(err==ESP_FAIL)
        // {
        //     ESP_LOGE(MESH_TAG,"FATAl AND FINAL alternative. Reboot %x",err);
        //     esp_restart();
        // }
        // printf("requesting connect retry #%d\n",retry++);
        uxBits=xEventGroupWaitBits(wifi_event_group, MQTT_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   pdMS_TO_TICKS(10000)); //explicit Connect Bit
        if(((uxBits & MQTT_BIT) ==MQTT_BIT) )       //only mqttbit connect 
        { 
            ESP_LOGI(MESH_TAG,"Reconnection successfull");
            //we are connected and subscribed YEAH
            err=esp_mqtt_client_disconnect(clientCloud);
            xEventGroupWaitBits(wifi_event_group, DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   portMAX_DELAY); //explicit disConnect Bit
            // esp_restart();  //sure option but kinda pussy solution
            err=esp_mqtt_client_stop(clientCloud);
            root_mqtt_app_start();
            delay(2000);
            xTimerStart(repeatTimer, 0 );       //start the mesh comms
            recoTaskHandle=NULL;
            vTaskDelete(recoTaskHandle);        //outselves...good job 
        }
    }
}

static void root_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    int err,cualq;
    mqttMsg_t mqttHandle;
    char *msg;
    char    eltopic[60];

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        err=esp_mqtt_client_subscribe(client,cmdQueue, 0);
        // err=esp_mqtt_client_subscribe(client,cmdBroadcast, 0);
        mqttf=true;
        sendMeterf=false; 
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupSetBits(wifi_event_group, DISCO_BIT);         // used as a CONNECT Mqtt and Subscribe in One
        xEventGroupClearBits(wifi_event_group,MQTT_BIT);
        delay(100);
        // mqttErrors++;
        // if(mqttErrors>MAXMQTTERRS)
        // {
        //     ESP_LOGW("MESH_TAG","Too many retries %d MQtt... rebooting",mqttErrors);
        //     esp_restart();
        // }
        mqttf=false;  
        sendMeterf=false; 
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        xEventGroupSetBits(wifi_event_group, MQTT_BIT);         // used as a CONNECT Mqtt and Subscribe in One
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
#ifdef MQTTSUB
        xEventGroupSetBits(wifi_event_group, PUB_BIT|DONE_BIT);//message sent bit
#endif
        mqttf=false;
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
#ifdef MQTTSUB
            esp_mqtt_client_unsubscribe(client, cmdQueue);//bit is set by unsubscribe
#else
            xEventGroupSetBits(wifi_event_group, PUB_BIT);//message sent bit
#endif
            break;
    case MQTT_EVENT_DATA:
            bzero(&mqttHandle,sizeof(mqttHandle));
            msg=(char*)calloc(event->data_len+5,1);
            memcpy(msg,event->data,event->data_len);
            mqttHandle.message=(uint8_t*)msg;
            mqttHandle.msgLen=event->data_len;

            if(event->data_len)
            {
                theMeter.setStatsBytesIn(event->data_len);
                theMeter.setStatsMsgIn();
                // ESP_LOGI(MESH_TAG,"TOPIC=%.*s",event->topic_len, event->topic);
                // ESP_LOGI(MESH_TAG,"DATA=%.*s %d", event->data_len, event->data, event->data_len);
                bzero(eltopic,sizeof(eltopic));
                memcpy(eltopic,event->topic,event->topic_len);
                if(xQueueSend( mqttQ,&mqttHandle,0 )!=pdTRUE)
                    ESP_LOGE(MESH_TAG,"Cannot add msgd mqtt");
                if(strcmp(eltopic,cmdQueue)==0)
                    esp_mqtt_client_publish(clientCloud, cmdQueue, "", 0, 0,1);//delete retained
                else
                    //hopefully he can do that and mqwtt is not already closed
                    if(strcmp(eltopic,cmdBroadcast))
                        esp_mqtt_client_publish(clientCloud, cmdBroadcast, "", 0, 0,1);//delete retained
            }
            //sendmeterf leave alone, it IS waiting for a message to be sent and Publish via Mqtt magr should reset it
            break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        xEventGroupClearBits(wifi_event_group,ERROR_BIT);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            if(!recoTaskHandle)
            {
                ESP_LOGW(MESH_TAG, "Transport error. ReconnetTask launched");
                err=esp_mqtt_client_disconnect(clientCloud);
                if(xTaskCreate(root_reconnectTask,"mqttreco",1024*4,NULL, 14,&recoTaskHandle)!=pdPASS)
                    ESP_LOGE(MESH_TAG,"Cannot create reconnect task");
            }
            else
                delay(5000);
    //        esp_restart();      //can not go on like that, reboot
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(MESH_TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            printf("Mqtt User/password wrong\n");
        } else {
            ESP_LOGW(MESH_TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
            sendMeterf=false;                                           
            xEventGroupSetBits(wifi_event_group, ERROR_BIT);         
        break;
    default:
        // ESP_LOGI(TAG, "MQTT Other event id:%d", event->event_id);
        break;
    }
}

static int findCommand(const char * cual)
{
	for (int a=0;a<MAXCMDS;a++)
	{
		if(strcmp(cmds[a].comando,cual)==0)
			return a;
	}
	return ESP_FAIL;
}

void meshSendTask(void *pArg)
{
    mqttSender_t meshMsg;
    mesh_data_t data;
    while(true)
    {
        if( xQueueReceive( meshQueue, &meshMsg, portMAX_DELAY ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
        {   
            // printf("Mesh send [%s] size[%d]\n",meshMsg.msg,meshMsg.lenMsg);
            data.proto = MESH_PROTO_BIN;
            data.tos = MESH_TOS_P2P;
            data.data=(uint8_t*)meshMsg.msg;
            data.size=meshMsg.lenMsg;
            int err = esp_mesh_send(NULL, &data, MESH_DATA_FROMDS, NULL, 0);
            if(meshMsg.msg)
                free(meshMsg.msg);
        }
    }
}

void root_emergencyTask(void *pArg)
{
    mqttSender_t emergencyMsg,mqttMsg;
    mesh_data_t data;

    while(true)
    {
        if( xQueueReceive( mqtt911, &emergencyMsg, portMAX_DELAY ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
        {   
            if(emergencyMsg.msg)
            {
                if(esp_mesh_is_root())
                {
                    emergencyMsg.code=NULL;
                    emergencyMsg.param=NULL;
                    // we are root send the Mqtt message
                    if(xQueueSend(mqttSender,&emergencyMsg,0)!=pdTRUE)     
                    {
                        ESP_LOGE(MESH_TAG,"Error queueing emergency msg");
                        if(emergencyMsg.msg)
                            free(emergencyMsg.msg);  //due to failure
                    }   
                    xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// start mqtt sender
                }
                else
                {
                    //send Mesh message to ROOT to send Emergency msg
                    data.proto = MESH_PROTO_BIN;
                    data.tos = MESH_TOS_P2P;
                    data.data=(uint8_t*)emergencyMsg.msg;
                    data.size=emergencyMsg.lenMsg+1;
                    int err = esp_mesh_send(NULL, &data, MESH_DATA_FROMDS, NULL, 0);
                    free(emergencyMsg.msg);
                }
            }
        }
        delay(1000);        //allow others some cpu time
    }
}
 
esp_err_t check_security(cJSON * cmd)
{
    uint32_t chaint=0,rsaint;
    char *ptr;
    if(!theConf.useSec)
        return ESP_OK;      //we are not using security checks for some reason
    char *chKey=(char*)malloc(32);
    if(chKey)
    {
        memset(chKey,0x32,32);    // jaja, blank fill 32 chars
        if(cmd)
        {   
            char *que=cJSON_PrintUnformatted(cmd);
            memcpy(chKey,que,strlen(que)>32?32:strlen(que));        //get the key first 32 chars
            // printf("Key [%s]\n",chKey);
            free(que);
            cJSON *challenge= 		cJSON_GetObjectItem(cmd,"cha");
            if(!challenge)
            {
                free(chKey);
                // printf("No cha\n");
                return ESP_FAIL;
            }
            chaint=strtoul(challenge->valuestring, &ptr, 16);           //from string to HEX int
            // printf("Challenge %0x\n",chaint);
            char *aca=(char*)calloc (100,1);
            aes_encrypt(SUPERSECRET,sizeof(SUPERSECRET),aca,chKey);
            memcpy((uint8_t*)&rsaint,aca,4);
            uint8_t *intt=(uint8_t*)&rsaint;
            memcpy(intt++,&aca[3],1);
            memcpy(intt++,&aca[2],1);
            memcpy(intt++,&aca[1],1);
            memcpy(intt,&aca[0],1);
            // printf("%02x%02x%02x%02x internal %x rsaint %x\n",aca[0],aca[1],aca[2],aca[3],chaint,rsaint);
            free(aca);
        }
        free(chKey);
        if(rsaint==chaint)
            return ESP_OK;
        else
            return ESP_FAIL;
    }
    return ESP_FAIL;
}

void root_mqttMgr(void *pArg)
{
    mqttMsg_t mqttHandle;
	cJSON 	*elcmd;
    while(true)
    {
        if( xQueueReceive( mqttQ, &mqttHandle, portMAX_DELAY ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
        {
            // ESP_LOGI(MESH_TAG,"Message In MQtt %s len %d",mqttHandle.message,mqttHandle.msgLen);
            elcmd= cJSON_Parse((char*)mqttHandle.message);		//plain text to cJSON... must eventually cDelete elcmd
			if(elcmd)
			{
                // printf("CmdIn parsable\n");
                cJSON *monton= 		    cJSON_GetObjectItem(elcmd,"cmdarr");
                if(monton)
                {
                    int son=cJSON_GetArraySize(monton);
                    // printf("cmds in %d\n",son);
                    for (int a=0;a<son;a++)
                    {
                        cJSON *cmditem 	=cJSON_GetArrayItem(monton, a);//next item
                        if(cmditem)
                        {
                            // printf("Cmd[%d]\n",a);
                            cJSON *order= 		cJSON_GetObjectItem(cmditem,"cmd");
                            if(order)
                            {
                                ESP_LOGI(MESH_TAG,"External cmd [%s]",order->valuestring);
                                int cual=findCommand(order->valuestring);
                                if(cual>=0)
                                {
                                    if(check_security(cmditem)==ESP_OK)     // he is in charge of sending warning to Central Control
                                    {
                                        (*cmds[cual].code)((void*)cmditem);	// call the cmd and wait for it to end
                                     }
                                    else
                                    ESP_LOGW(MESH_TAG,"Invalid security challenge");
                                }
                                else    
                                    ESP_LOGE(MESH_TAG,"Invalid cmd received %s",order->valuestring);
                            }
                            else
                                ESP_LOGE(MESH_TAG,"No order received %s",a);
                        }
                    }
                    cJSON_Delete(elcmd);
                }
                else
                    ESP_LOGE(MESH_TAG,"No cmdarr received");
            }
            else
                ESP_LOGE(MESH_TAG,"Cmd received not parsable [%s]",mqttHandle.message);

            if(mqttHandle.message)
                free(mqttHandle.message);
        }
    }
}

/// to get ssl certificate 
//                                                <-------- Site/Port -->
// echo "" | openssl s_client -showcerts -connect m13.cloudmqtt.com:28747 | sed -n "1,/Root/d; /BEGIN/,/END/p" | openssl x509 -outform PEM >hive.pem

static esp_mqtt_client_handle_t root_setupMqtt()
{
    char who[20],mac[8];
    esp_base_mac_addr_get((uint8_t*)mac);
    bzero(who,sizeof(who));
    sprintf(who,"Meterserver%d",theConf.controllerid);
    // printf("MQTT NodeId %s\n",who);
    int sslen=cert_end-cert_start;
    bzero((void*)&mqtt_cfg,sizeof(mqtt_cfg));
    mqtt_cfg.broker.address.uri=  					            theConf.mqttServer;  // pem certificate for m13.cloudmqtt.com:28747 match with hivessl.pem
    // mqtt_cfg.broker.verification.certificate=                   (const char*)cert_start;
    // mqtt_cfg.broker.verification.certificate_len=                sslen;
    mqtt_cfg.broker.verification.certificate=                   theConf.mqttcert;
    mqtt_cfg.broker.verification.certificate_len=               theConf.mqttcertlen;
    mqtt_cfg.credentials.client_id=				                who;
    mqtt_cfg.credentials.username=					            theConf.mqttUser;
    mqtt_cfg.credentials.authentication.password=		        theConf.mqttPass;
    mqtt_cfg.network.disable_auto_reconnect=                    true;
    mqtt_cfg.buffer.size=                                       MQTTBIG;         
   
// printf("Mqtt client [%s] user [%s] pass [%s] uri [%s] \n",
//          mqtt_cfg.credentials.client_id,mqtt_cfg.credentials.username,mqtt_cfg.credentials.authentication.password,
//                     mqtt_cfg.broker.address.uri );


// NOTICE
// WITHOUT ssl encryption average3 message of 8 meters takes aprox 450ms
// WITH ssl encryption it 1250 !!! for your information IMPORTANT
// wss takes 2000ms

    clientCloud = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(clientCloud, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, root_mqtt_event_handler, NULL);
    return clientCloud;
}

static void root_mqtt_app_start(void)
{
BaseType_t xReturned;

        xReturned=xTaskCreate(&root_mqttMgr,"mqtt",4096*2,NULL, 10, &mqttMgrHandle);      //receiving commands
        if(xReturned!=pdPASS)
            ESP_LOGE(MESH_TAG,"Fail to launch Mgr");
        xReturned=xTaskCreate(&root_mqtt_sender,"mqttsend",1024*10,NULL, 14, &mqttSendHandle);
        if(xReturned!=pdPASS)
            ESP_LOGE(MESH_TAG,"Fail to launch Sender");

}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    esp_err_t err;


    mesh_addr_t id = {0,};
    static uint8_t last_layer = 0;

    switch (event_id) {
    case MESH_EVENT_STARTED: {
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED: {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 child_connected->aid,
                 MAC2STR(child_connected->mac));
                         esp_mesh_get_id(&id);
                         memcpy(&id.addr,&child_connected->mac,6);
        if( esp_mesh_is_root())
        {
            err= root_send_data_to_node(id);      //everybody gets updated for the time being
            if(err!=ESP_OK) 
                ESP_LOGE(MESH_TAG,"Send SSID Failed");
        }

    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED: {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 child_disconnected->aid,
                 MAC2STR(child_disconnected->mac));
        if(sendMeterf)  //very important to ask
        {
            ESP_LOGI(MESH_TAG,"Disconnect while sending data. Retry");
            //timeout will manage to send whomever was available
            // this station NEVER sent the data so no RAM to free
        }
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND: {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 no_parent->scan_times);

    }
    /* TODO handler for the failure */    
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        last_layer = mesh_layer;
        mesh_netifs_start(esp_mesh_is_root());
    }
    break;
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 disconnected->reason);
                 //use this as LOST_IP
        if(esp_mesh_is_root())
        {
            hostflag=false;
            mqttf=false;
            theMeter.setStatsStaDiscos();
        }
        mesh_layer = esp_mesh_get_layer();
        mesh_netifs_stop();
    }
    break;
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
        mesh_layer = layer_change->new_layer;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
    }
    break;
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 MAC2STR(root_addr->addr));
    }
    break;
    case MESH_EVENT_VOTE_STARTED: {
        mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 vote_started->attempts,
                 vote_started->reason,
                 MAC2STR(vote_started->rc_addr.addr));
    }
    break;
    case MESH_EVENT_VOTE_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    }
    case MESH_EVENT_ROOT_SWITCH_REQ: {
        mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 switch_req->reason,
                 MAC2STR( switch_req->rc_addr.addr));
    }
    break;
    case MESH_EVENT_ROOT_SWITCH_ACK: {
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
    }
    break;
    case MESH_EVENT_TODS_STATE: {
        mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    }
    break;
    case MESH_EVENT_ROOT_FIXED: {
        mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 root_fixed->is_fixed ? "fixed" : "not fixed");
    }
    break;
    case MESH_EVENT_ROOT_ASKED_YIELD: {
        mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(root_conflict->addr),
                 root_conflict->rssi,
                 root_conflict->capacity);
    }
    break;
    case MESH_EVENT_CHANNEL_SWITCH: {
        mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    }
    break;
    case MESH_EVENT_SCAN_DONE: {
        mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 scan_done->number);
    }
    break;
    case MESH_EVENT_NETWORK_STATE: {
        mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 network_state->is_rootless);
    }
    break;
    case MESH_EVENT_STOP_RECONNECTION: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    }
    break;
    case MESH_EVENT_FIND_NETWORK: {
        mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 find_network->channel, MAC2STR(find_network->router_bssid));
    }
    break;
    case MESH_EVENT_ROUTER_SWITCH: {
        mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    }
    break;
    default:
        ESP_LOGI(MESH_TAG, "Mesh unknown id:%d", event_id);
        break;
    }
} // MESH_TAG ESPM_E63C64

void send_internal_emergency(char * msg, uint32_t err)
{
        mqttSender_t meshMsg;
        bzero(&meshMsg,sizeof(meshMsg));            //have to zero it for the callback and param

        cJSON *root=cJSON_CreateObject();
        if(root)
        {
            cJSON_AddStringToObject(root,"cmd","911");
            cJSON_AddStringToObject(root,"msg",msg);
            cJSON_AddNumberToObject(root,"err",err);
            char *intmsg=cJSON_PrintUnformatted(root);
            if(intmsg)
            {
                printf("Emergency [%s]\n",intmsg);
                meshMsg.queue=emergencyQueue;
                meshMsg.msg=intmsg;
                meshMsg.lenMsg=strlen(intmsg);
                if(xQueueSend(meshQueue,&meshMsg,0)!=pdPASS)
                    ESP_LOGE(MESH_TAG,"Cannot queue fram");   //queue it
            }
            cJSON_Delete(root);
        }

}



void init_process()
{
    int err;

	flashSem= xSemaphoreCreateBinary();
	xSemaphoreGive(flashSem);

	tableSem= xSemaphoreCreateBinary();
	xSemaphoreGive(tableSem);

//nvs to access config file
     err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
		ESP_LOGI(MESH_TAG,"No free pages erased!!!!");
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

        //read flash        
    read_flash();
    if(theConf.centinel!=CENTINEL)
    {
        ESP_LOGI(MESH_TAG,"Centinel invalid check. Erase Config");
        erase_config();
    }
    esp_log_level_set("*",(esp_log_level_t)theConf.loglevel);   //set log level

	theConf.lastResetCode=esp_rom_get_reset_reason(0);              //store reset reason and reboot count
    theConf.bootcount++;
    write_to_flash();           
// end nvs

//lots of flags
//todo try to avoid them
    showHandle=NULL;
    meshf=false;
    mqttErrors=0;
    BASETIMER=theConf.baset;         // miliseconds 1 minute
    // mesh stuff 
    mesh_layer = -1;
    mesh_started=false;
    // mesh id as we cannot use static variable configuration
    memset(MESH_ID,0x77,6);
    MESH_ID[5]=FIXEDMESH+1;
    esp_log_buffer_hex(MESH_TAG,&MESH_ID,6);
    mqttf=false;
    memset(&GroupID.addr ,0xff,6);
    sendMeterf=false;
    hostflag=false;
    loadedf=false;
    firstheap=false;
    acumheap=0;
    clientCloud=NULL;
    lastKnowCount=0;

//cmd and info queue names derived form the Config so do it now
    sprintf(cmdQueue,"%s/%d/cmd",QUEUE,theConf.controllerid);
    sprintf(infoQueue,"%s/%d/info",QUEUE,theConf.controllerid);
    // sprintf(cmdQueue,"%s/%d/%d/%d/%d/%d/cmd",QUEUE,theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,theConf.controllerid);
    // sprintf(infoQueue,"%s/%d/%d/%d/%d/%d/info",QUEUE,theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,theConf.controllerid);
    sprintf(emergencyQueue,"%s/911",QUEUE,theConf.controllerid);
    sprintf(cmdBroadcast,"%s/broadcast",QUEUE);
    sprintf(discoQueue,"%s/disco",QUEUE);
    sprintf(installQueue,"%s/install",QUEUE);

// internal mesh commands 

    int x=0;
    strcpy(internal_cmds[EMERGENCY],        "911");
    strcpy(internal_cmds[BOOTRESP],         "bootresp");
    strcpy(internal_cmds[SENDMETRICS],      "sendmetrics");
    strcpy(internal_cmds[METERSDATA],       "meterdata");
    strcpy(internal_cmds[LOCKMETER],        "lock");
    strcpy(internal_cmds[CONFIRMLOCK],      "confirm");
    strcpy(internal_cmds[CONFIRMLOCKERR],   "confError");
    strcpy(internal_cmds[INSTALLATION],     "install");
    strcpy(internal_cmds[REINSTALL],        "reinstall");
    strcpy(internal_cmds[CONFIRMINST],      "installconf");
    strcpy(internal_cmds[FORMAT],           "format");     //erases all fram must reinstall from scratch
    strcpy(internal_cmds[UPDATEMETER],      "update");
    strcpy(internal_cmds[ERASEMETRICS],     "erase");     //erases all metrics
    strcpy(internal_cmds[MQTTMETRICS],      "mqttmetrics");     //send all mesh metrics
    strcpy(internal_cmds[METRICRESP],       "metriscresp");     //sendhost required metrics
    strcpy(internal_cmds[SHOWDISPLAY],      "display");     //turn on the display


//external commands via mqtt
    x=0;            //reset counter
	strcpy((char*)&cmds[x].comando,         "format");			        cmds[x].code=cmdFormat;			strcpy((char*)&cmds[x].abr,         "FRMT");		
	strcpy((char*)&cmds[++x].comando,       "netw");		            cmds[x].code=cmdNetw;			strcpy((char*)&cmds[x].abr,         "NETW");		
	strcpy((char*)&cmds[++x].comando,       "mqtt");		            cmds[x].code=cmdMQTT;			strcpy((char*)&cmds[x].abr,         "MQTT");		
	strcpy((char*)&cmds[++x].comando,       "lock");		            cmds[x].code=cmdLock;			strcpy((char*)&cmds[x].abr,         "LOCK");		
	strcpy((char*)&cmds[++x].comando,       "update");		            cmds[x].code=cmdUpdate;			strcpy((char*)&cmds[x].abr,         "UPDT");		
	strcpy((char*)&cmds[++x].comando,       "erase");		            cmds[x].code=cmdEraseMetrics;   strcpy((char*)&cmds[x].abr,         "ERAS");		
	strcpy((char*)&cmds[++x].comando,       "mqttmetrics");		        cmds[x].code=cmdSendMetrics;    strcpy((char*)&cmds[x].abr,         "METR");		
	strcpy((char*)&cmds[++x].comando,       "ota");		                cmds[x].code=cmdHostOTA;        strcpy((char*)&cmds[x].abr,         "HOTA");		
    strcpy((char*)&cmds[++x].comando,       "setota");		            cmds[x].code=cmdSetOTA;         strcpy((char*)&cmds[x].abr,         "SOTA");		
    strcpy((char*)&cmds[++x].comando,       "display");		            cmds[x].code=cmdDisplay;        strcpy((char*)&cmds[x].abr,         "DISP");		
  
//gpios

    ssignal=BEATPIN;            //our meter connecction pin

    bzero(&io_conf,sizeof(io_conf));
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;    //it will have an ext3ernal pullup anyway
    io_conf.pin_bit_mask = (1ULL<<RELAY);
    gpio_set_level((gpio_num_t)RELAY,1);        //before configuring it decide level we want it to be at start, in this case HIGH
    err=gpio_config(&io_conf);

	io_conf.pin_bit_mask = ((1ULL<<WIFILED) );     //output pins 
    gpio_set_level((gpio_num_t)WIFILED,0);          //before configuring it decide level we want it to be at start, in this case LOW
	gpio_config(&io_conf);
    
    //input pins do not need it anymore I think
	// io_conf.mode = GPIO_MODE_INPUT;
	// io_conf.pull_up_en =GPIO_PULLUP_ENABLE;
	// io_conf.pull_down_en =GPIO_PULLDOWN_DISABLE;
    // io_conf.intr_type = GPIO_INTR_DISABLE;
	// io_conf.pin_bit_mask = (1ULL<<ERASE);          
	// gpio_config(&io_conf);

    // aes init     for encryptions
    esp_aes_init( &actx );

//queues

    mqtt911 = xQueueCreate( MAXNODES, sizeof( mqttMsg_t ) );
    if(!mqtt911)
        ESP_LOGE(MESH_TAG,"Failed queue 911");
    meshQueue = xQueueCreate( MAXNODES, sizeof( mqttMsg_t ) );
    if(!meshQueue)
        ESP_LOGE(MESH_TAG,"Failed queue meshSend");
    mqttQ = xQueueCreate( MAXNODES, sizeof( mqttMsg_t ) );
    if(!mqttQ)
        ESP_LOGE(MESH_TAG,"Failed queue Cmd");
    mqttSender = xQueueCreate( MAXNODES, sizeof( mqttSender_t ) );
    if(!mqttSender)
        ESP_LOGE(MESH_TAG,"Failed queue Sender");


//timing stuff and timers

    uint32_t permanent_time=(theConf.totalnodes/theConf.conns)*EXPECTED_DELIVERY_TIME;
    if(permanent_time==0)
    {
        ESP_LOGI(MESH_TAG,"Permanete time 0...default to 500");
        permanent_time=500;
    }
    if (theConf.repeat<1 )
        theConf.repeat=1;

    sendMeterTimer=xTimerCreate("SendM",pdMS_TO_TICKS(MESHTIMEOUT),pdFALSE,NULL, []( TimerHandle_t xTimer){ xEventGroupSetBits(otherGroup,REPEAT_BIT);});    // every 10secs for now -> use lambda
    repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS(permanent_time*theConf.repeat),pdFALSE,( void * ) 0, root_collect_meter_data);    //no repeat, manually start it -> to big for lambda
    confirmTimer=xTimerCreate("Confirm",pdMS_TO_TICKS(CONFIRMTIMER),pdFALSE,( void * ) 0,[] (TimerHandle_t xTimer)
{
    char *cualMID;

            cJSON *root=cJSON_CreateObject();
            if(root)
            {
                cualMID = ( char* ) pvTimerGetTimerID( xTimer );

                cJSON_AddStringToObject(root,"cmd",internal_cmds[6]);
                cJSON_AddStringToObject(root,"mid",cualMID);
                cJSON_AddNumberToObject(root,"status",METER_NOT_FOUND);

                char *intmsg=cJSON_PrintUnformatted(root);
                if(intmsg)
                {
                    ESP_LOGW(MESH_TAG,"Confirm time out for [%s] ",cualMID);
                    root_send_confirmation_central(intmsg,strlen(intmsg),discoQueue);
                    free(intmsg);
                }
                cJSON_Delete(root);
            }
            else
                ESP_LOGE(MESH_TAG,"No RAM for Confirm timeout");
});
    
    beatTimer=xTimerCreate("beats",pdMS_TO_TICKS(BEATTIMER),pdFALSE,( void * ) 0, [] (TimerHandle_t xTimer){theMeter.saveMeter();});    //no repeat, manually start it -> use lambda


//keyboard commands

#ifdef DEBB
    xTaskCreate(&kbd,"kbd",1024*10,NULL, 5, NULL); 	        // keyboard commands
#endif
    //loggin stuff?
#ifdef LOGOPT
        logFileInit();              //log file init
    timeval localtime;

    theConf.lastKnownDate=theMeter.getReservedDate();
                localtime.tv_sec=theConf.lastKnownDate;
                localtime.tv_usec=0;                                      // no connection most likely
                settimeofday(&localtime,NULL);
#endif

//event group bits for wifdi and other uses
    wifi_event_group = xEventGroupCreate();
    otherGroup = xEventGroupCreate();           //other group
}

void erase_config()
{
    ESP_LOGI(MESH_TAG,"Erase config");
    // wifi_prov_mgr_reset_provisioning();
    esp_wifi_restore();
    nvs_flash_erase();
    nvs_flash_init();
    bzero(&theConf,sizeof(theConf));
    // theConf.controllerid = 1 + (esp_random() % 999999);       //will be displayed as the node for configuration
    theConf.centinel=CENTINEL;
    
    //default mqtt server certificate and user/pass
    strcpy(theConf.mqttServer,"mqtts://moose.rmq.cloudamqp.com:8883");
    strcpy(theConf.mqttUser,"yqsgcxzu:yqsgcxzu");
    strcpy(theConf.mqttPass,"d1nJEL8K-fHyox70LqvszcSqbP71ats8");
    strcpy(theConf.mqttcert,(const char*)cert_start);       //copy default cert and len to configuration
    theConf.mqttcertlen=cert_end-cert_start;
    strcpy(theConf.OTAURL,"http://64.23.180.233/metermgr.bin");
    int ssllen=cert_end-cert_start;
    memcpy(theConf.mqttcert,cert_start,ssllen);
    theConf.mqttcertlen=ssllen;

    time((time_t*)&theConf.bornDate); 
    theConf.loglevel=3;
    theConf.baset=10;
    theConf.repeat=25; // change before production
    theConf.totalnodes=EXPECTED_NODES;
    theConf.conns=EXPECTED_CONNS;

    theConf.gFreq=              ADCFREQ;
    theConf.biasAmp=            ADCBIAS;
    theConf.gVref=              ADCVREF;
    theConf.maxAmp=             ADCMAXAMP;
    theConf.minAmp=             ADCMINAMP;
    
    strcpy(theConf.kpass,"csttpstt");
    write_to_flash();

}

// Routine to send mqtt messages
// VERY sensitive to timeouts in 2 places 
// first the WIFI network must be active in order for 
// MQTT service to be active
// now ity start manually the repeat timer, to avoid timer conflicts timeout timer and repeat timer
// so, this routine now controls when to fire the repeat timer again

void root_mqtt_sender(void *pArg)        // MQTTT data sender task
{
    mqttSender_t        mensaje;
    uint32_t            startMqtt,endMqtt;
    int                 err,whost;
    EventBits_t         uxBits;
    int                 msgid;
    time_t              now;

    xEventGroupClearBits(wifi_event_group, SENDMQTT_BIT);	// clear bit to wait on
    while(true)         //task is forever
    {
        alla:
        while(!hostflag)            //check if host is active...hum not solid
        {
            delay(300);             // if no host, no sending...wait for it display warning after X retries
            whost++;
            if(whost>10)
            {
                ESP_LOGW(MESH_TAG,"Waiting for host");
                whost=0;
            }
        }
        xEventGroupWaitBits(wifi_event_group,SENDMQTT_BIT,pdTRUE,pdFALSE,portMAX_DELAY);    //wait forever, this is the starting gun, flag will be cleared
        // ESP_LOGI(MESH_TAG,"Check host: %s",hostflag?"connected":"disconnected");
        startMqtt=xmillis();
        ESP_LOGI(MESH_TAG,"Mqttsender establish");
        uxBits=xEventGroupClearBits(wifi_event_group, MQTT_BIT); //explicit Connect Bit
        //dynamic connect stategy
        if(!clientCloud)
            clientCloud=root_setupMqtt();
        if(clientCloud)     //in case we cannot get a connection.... super secure programming
        {
            xEventGroupClearBits(wifi_event_group,MQTT_BIT|ERROR_BIT|DISCO_BIT);        // clear bits
            err=esp_mqtt_client_start(clientCloud);         //start a MQTT connection, wait for it in MQTT_BIT
            uxBits=xEventGroupWaitBits(wifi_event_group, MQTT_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   portMAX_DELAY); //explicit Connect Bit, can get stuck here
            if(((uxBits & MQTT_BIT) ==MQTT_BIT) && err!=ESP_FAIL)       //only mqttbit connect other skip sending
            {   // great, something to do
                ESP_LOGI(MESH_TAG,"Mqtt Sender connected");

                while(true) //send all items in queue if Mqtt is active
                {
                    // mensaje.code=NULL;mensaje.param=NULL;
                    if( xQueueReceive( mqttSender, &mensaje,  pdMS_TO_TICKS(MQTTSENDERWAIT) ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
                    {
                        if(mensaje.msg)
                        {
                            if(mensaje.queue==emergencyQueue){
                            ESP_LOGI(MESH_TAG,"Publish sender len %d buff [%s]",mensaje.lenMsg,mensaje.msg);
                            }
                            // ESP_LOG_BUFFER_HEX(MESH_TAG,mensaje.msg,mensaje.lenMsg);
                            msgid=esp_mqtt_client_publish(clientCloud,(char*) mensaje.queue, (char*)mensaje.msg,mensaje.lenMsg, 1,0);
                            // ESP_LOGI(MESH_TAG,"Publish msgid %x len %d",msgid,mensaje.lenMsg);
                            if(msgid==ESP_FAIL)   //msg_id cannot be negative, means error
                            {
                                ESP_LOGE(MESH_TAG,"Publish failed %x",msgid);
                                if(mensaje.msg)
                                    free(mensaje.msg); 
                                if(mensaje.param)
                                    free(mensaje.param);                                     
                                break;      //wait for next time slot....should free thinsgs up
                            }
                            else
                            {       // wait for published bit or error
                                uxBits=xEventGroupWaitBits(wifi_event_group, PUB_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   portMAX_DELAY); //explicit Connect Bit
                                if(((uxBits & PUB_BIT) !=PUB_BIT))   
                                    ESP_LOGE(MESH_TAG,"Pub failed Error or Disco %x",uxBits);
// TODO mejorar codigo de error. Se puede trabar aqui
                                xEventGroupClearBits(wifi_event_group,PUB_BIT|DISCO_BIT|ERROR_BIT);
                                theMeter.setStatsMsgOut();  //increment count
                                theMeter.setStatsBytesOut(mensaje.lenMsg);
                                endMqtt=xmillis();          
                                //see if callback given
                                if(mensaje.code)
                                {
                                    // printf("Callback %p param %ld\n",mensaje.code,mensaje.param);
                                    (*mensaje.code)((void*)mensaje.param);	// call back should be fast
        //free param MUST be a pointer to anything
                                }
                                if(mensaje.msg)
                                    free(mensaje.msg);          //BIGF responsability if not heap collapses
                                if(mensaje.param)
                                    free(mensaje.param); 
                                // msgout++;
                                ESP_LOGI(MESH_TAG,"MsgLen %d Msgs sent %d msec %d",mensaje.lenMsg,theMeter.getStatsMsgOut(),endMqtt-startMqtt);
                                startMqtt=xmillis();
                                if(((uxBits & ERROR_BIT) ==ERROR_BIT) )
                                {
                                    ESP_LOGE(MESH_TAG,"Error detected mqttsender");
                                    break;
                                }
                                if ((theMeter.getStatsMsgOut() % SAVEDATE)==0)
                                {
                                        time(&now);
                                        theMeter.setReservedDate(now);
                                }
                            }
                        }
                        else
                            ESP_LOGE(MESH_TAG,"No message to send but Process activated");
                    }               
                    else        // queue empty, now close
                    {
                        sendMeterf=false;   //best way yet to sync sending and done sending 
                        // ESP_LOGI(MESH_TAG, "Closing connection");
                        err=esp_mqtt_client_disconnect(clientCloud);
                        xEventGroupWaitBits(wifi_event_group, DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   portMAX_DELAY); //explicit Connect Bit
                        err=esp_mqtt_client_stop(clientCloud);
                        ESP_LOGI(MESH_TAG,"Conn closed and Queue empty");
                        break;  //get out of send loop , all msgs were sent           
                    }
                }
            }
            else  
            {   
                // some problem, disconnect or error, in etiher case Task was released and will start loop again
                ESP_LOGE(MESH_TAG,"Did not get connect bit or error %x\n",uxBits);
                sendMeterf=false;
                delay(100);  //in case its very fast
                //TODO need to free ram due to no connection and data stored
            }
            if( xTimerStart(repeatTimer, 0 ) != pdPASS )
                ESP_LOGE(MESH_TAG,"Repeat Timer mqtt saender failed");
        }
        else    // NO connection to MQTT, but Host is active
        {
            sendMeterf=false;           //reset flag so we can try again, even if it suspicious that it could not open the connection
            ESP_LOGE(MESH_TAG,"Cannot connect to ClientCloud");
        }
        delay(100);
    }
}   //should never leave this while its a Task


/**
 * ? que sera
 * ! warning
 * * Important
 * TODO: algo
 * @param xTimer este parametro
 */
//  void root_firstCallback( TimerHandle_t xTimer )
//  {

//     int ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );
//     if(!theConf.meterconf)  
//         return;

//     // collect_meter_data(NULL);           // first time 
    
//     // repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS(permanent_time*theConf.repeat),pdFALSE,( void * ) ulCount, collect_meter_data);    //no repeat, manually start it
//     // repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS(permanent_time*theConf.repeat),pdTRUE,( void * ) ulCount, collect_meter_data);    // every hour for now
//     if( xTimerStart(repeatTimer, 0 ) != pdPASS )
//         ESP_LOGE(MESH_TAG,"Repeat Timer failed");
//  }

void root_set_senddata_timer()
{
    // get our defined time slot based on the controllerid and expected publish time... in seconds
    // mqttSlots will be defined by default and modified by Host via any cmd message with SLOT primitive in Json msg
       
    time_t      now;
    struct tm   timeinfo;
    int         cuando=0;

    time(&now);
    // printf("Just now %lu %s",(int)now,ctime(&now));

    localtime_r(&now, &timeinfo);
    timeinfo.tm_hour=0;
    timeinfo.tm_min=0;
    timeinfo.tm_sec=0; //midnight hour
    time_t midnight = mktime(&timeinfo);
    // printf("Midnight %lu [%s]\n",(int)midnight,ctime(&midnight));
    int cycles=theConf.totalnodes/theConf.conns;
    
    int secs_from_midnight=int(now-midnight);
    // printf("Secs from midnight %d cycles %d\n",secs_from_midnight,cycles);

    int currCycle= (secs_from_midnight/EXPECTED_DELIVERY_TIME) % cycles;
    int passedCycle =(secs_from_midnight/EXPECTED_DELIVERY_TIME) / cycles;
    // printf("Passedcycles %d curcycle %d\n",passedCycle,currCycle);
    int time_to_next_cycle;
    if(currCycle<theConf.mqttSlots)
    {
        time_to_next_cycle =(theConf.mqttSlots-currCycle)*EXPECTED_DELIVERY_TIME;
    }
    else
    {
        time_to_next_cycle =(cycles-currCycle+theConf.mqttSlots)*EXPECTED_DELIVERY_TIME;

    }

    printf("Now %d Mid night %d secMid %d Secs to resync %d\n",(int)now,
           (int)midnight,secs_from_midnight,time_to_next_cycle);
    
    if(esp_mesh_is_root())
    {
        if(theConf.baset<1)
            theConf.baset=10;
        firstTimer=xTimerCreate("Timer",pdMS_TO_TICKS(time_to_next_cycle*theConf.baset),pdFALSE,( void * ) theConf.mqttSlots,
        [](TimerHandle_t xTimer)
        {

    int ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );
    if(!theConf.meterconf)  
        return;

    if( xTimerStart(repeatTimer, 0 ) != pdPASS )
        ESP_LOGE(MESH_TAG,"Repeat Timer failed");
        }
        );   //will be called onc eMesh and MQTT established
        if( xTimerStart(firstTimer, 0 ) != pdPASS )
            ESP_LOGE(MESH_TAG,"First Timer failed");
    }
}

void post_root()
{
    xTaskCreate(&root_sntpget,"sntp",4096,NULL, 10, NULL); 	        // get real time
}

void check_installation()
{
    mesh_data_t         data;

    if(theConf.meterconf==1)        //for redundancy
    {
        //queue msg to meshsender that we have been installed so it can be relayed to Central wioth its conditions
        if(strlen(theConf.instMsg)>10)
        {
            char *intmsg=(char*)calloc(theConf.instMsglen+1,1);        //need that last 0
            if(intmsg)
            {
                memcpy(intmsg,theConf.instMsg,theConf.instMsglen);
                // printf("To send confirmation [%s] len :%d\n",intmsg,theConf.instMsglen);
                data.data   =(uint8_t*)intmsg;
                data.size   =theConf.instMsglen;
                data.proto  = MESH_PROTO_BIN;
                data.tos    = MESH_TOS_P2P;

            //need to send a broadcast message to all stations so they can lock
            // this ID meter to whatever state was sent
            int  err= esp_mesh_send( NULL, &data, MESH_DATA_P2P, NULL, 1);       //to root
            if(err)
                ESP_LOGE(MESH_TAG,"Send Installation failed %x",err);
            free(intmsg); 
            }
        }
    }
}


void ip_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data)
{
    if (event_id ==IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_current_ip.addr = event->ip_info.ip.addr;
        theMeter.setStatsStaConns();
    #if !CONFIG_MESH_USE_GLOBAL_DNS_IP
        esp_netif_t *netif = event->esp_netif;
        esp_netif_dns_info_t dns;
        ESP_ERROR_CHECK(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
        mesh_netif_start_root_ap(esp_mesh_is_root(), dns.ip.u_addr.ip4.addr);
    #endif
        hostflag=true;
        gpio_set_level((gpio_num_t)WIFILED,1);

        if (esp_mesh_is_root() && !loadedf) 
        {
            loadedf=true;
            meshf=true;
            root_mqtt_app_start();
            post_root();
            xTaskCreate(&root_emergencyTask,"e911",2048,NULL, 5, NULL);
            xTaskCreate(&blinkRoot,"root",1024,NULL, 5, &blinkHandle);
        }
        if(theConf.meterconf==1)
            check_installation();
        xTaskCreate(&meshSendTask,"msend",4096,NULL, 10, NULL); 	            //now that its active, you can send any queued messages    

    }
    if(event_id ==IP_EVENT_STA_LOST_IP)
    {
       ESP_LOGI(MESH_TAG,"Lost ip");    //never gets called but Parent disconnect does
        hostflag=false;        
        gpio_set_level((gpio_num_t)WIFILED,0);

    }

}

void blinkSSID(void *pArg)
{
    uint32_t cuanto=(uint32_t)pArg;
    while(true)
    {
        gpio_set_level((gpio_num_t)WIFILED,1);
        gpio_set_level((gpio_num_t)BEATPIN,0);
        delay(cuanto);
        gpio_set_level((gpio_num_t)WIFILED,0);
        gpio_set_level((gpio_num_t)BEATPIN,1);
        delay(cuanto);
    }
}

static void wifi_event_handler_ap(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(MESH_TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        xTaskCreate(&blinkSSID,"bssid",4096,(void*)SSIDBLINKTIME, 5, &ssidHandle); 	        //SSID Blink

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(MESH_TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);// test remote
    if(ssidHandle)
    {
        gpio_set_level((gpio_num_t)BEATPIN,1);
        vTaskDelete(ssidHandle);
        ssidHandle=NULL;
    }
    }
    else if (event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
}

void wifi_init_network()
{
    // char            apssid[32],appsw[10];
    uint8_t         mac[6];
    wifi_config_t   wifi_config;

    bzero(&wifi_config,sizeof(wifi_config));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler_ap,
                                                        NULL,
                                                        NULL));

    esp_wifi_get_mac(WIFI_IF_AP,mac);
    bzero(apssid,sizeof(apssid));
    sprintf(apssid,"%s%02X%02X%02X\0",APPNAME, mac[3],mac[4],mac[5]);
    // printf("%s%02X%02X%02X%02X%02X%02X\0",APPNAME, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    strcpy(appsw,"csttpstt");   //Data is protected via Challenge Id. This passw should be "irrelevant"
    // printf("Config AP [%s] Passw [%s]\n",apssid,appsw);
    memcpy(&wifi_config.ap.ssid,apssid,strlen(apssid));
    memcpy(&wifi_config.ap.password,appsw,strlen(appsw));
    wifi_config.ap.ssid_len = strlen(apssid);
    wifi_config.ap.channel = 4;
    wifi_config.ap.max_connection = 1;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    //start web server
    start_webserver();      //Root or NRT case always start the Webserver to configure Meters
}

void meter_configure()
{
    mqttSender_t    mensaje;
    char            tmp[20];

    // if(theConf.ptch)
    // {
        // printf("Start timer\n");
        if( xTimerStart(reconfTimer, 0 ) != pdPASS )
            ESP_LOGI(MESH_TAG,"Repeat Timer failed to start");
    // }
    wifi_init_network();
    // seed the CID base code for Configuration
    int cid= (esp_random() % 99999999);
    sprintf(tmp,"%d",cid);
    printf("SSID %s Node %s\n",apssid,tmp);
    theConf.cid=cid;
    write_to_flash();
    while(true)
    {
        delay(1000);        // Webserver will restart after configuration or timeout
    }
}


void start_mesh()
{
    int             err;
    char            missid[30],mipassw[18];

    // MESH sequence start
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /*  crete network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(mesh_netifs_init(mesh_manager));
    wifi_init_config_t configg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&configg));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_LOST_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    // // printf("SSID %s password %s\n",config.ap.ssid,config.ap.password);

    // strcpy(missid,"Door");
    // strcpy(mipassw,"csttpstt");
    // bzero(missid,sizeof(missid));
    // bzero(mipassw,sizeof(mipassw));
    if(strlen(theConf.thessid)==0)
    {
        strcpy(missid,"RSNNetFlix");
        strcpy(mipassw,"csttpstt"); //8 chars at least
        strcpy(theConf.thessid,"RSNNetFlix");
        strcpy(theConf.thepass,"csttpstt");
        write_to_flash();
    }
    else
    {
        strcpy(missid,theConf.thessid);
        strcpy(mipassw,theConf.thepass);
    }
// nada
// printf("Missid [%s] pass [%s]\n",missid,mipassw);
        /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
    ESP_ERROR_CHECK(esp_mesh_set_root_healing_delay(5000)); //set healing delay time to 5 secs
    ESP_ERROR_CHECK(esp_mesh_send_block_time(30000));       //as in the example

    ESP_ERROR_CHECK(esp_mesh_disable_ps());
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);       //was setup in init_vars

    cfg.channel = 0;        //has to be 0 NO IDEA WHY
    
    //Router credentials STA
    cfg.router.ssid_len =strlen( missid);
    memcpy((uint8_t *) &cfg.router.ssid, missid, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, mipassw,strlen(mipassw));
   
   // softAP credientials
    // printf("cfg ssid [%s] password [%s]\n",cfg.router.ssid,cfg.router.password);
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode((wifi_auth_mode_t)CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    cfg.mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, "csttpstt",8);            //Only password required
    // printf("Starting Mesh ssid [%s]\n",(char*)cfg.router.ssid);
   err=esp_mesh_set_config(&cfg);
    if (err)
    {
        ESP_LOGE(MESH_TAG,"Init Mesh err %x");
        if(err==0x4008)
        {
            ESP_LOGE(MESH_TAG,"pasword len error");
            erase_config();
            esp_restart();
        }
    }
    /* mesh start */
    // Broadcast address setup
    //set a global flag that MESH has started. No esp_mesh_get-running or similar call so use flag
    esp_err_t errReturn = esp_mesh_set_group_id(&GroupID, 1);
    if(errReturn)
        ESP_LOGI(MESH_TAG,"Failed to resgister Mesh Broadcast");

     esp_mesh_set_topology(MESH_TOPO_CHAIN);
    ESP_ERROR_CHECK(esp_mesh_set_ie_crypto_funcs(NULL));

    ESP_ERROR_CHECK(esp_mesh_start());

    mesh_started=true;  //for kbd and other aux routines
    ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s\n",  esp_get_free_heap_size(),
             esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed");
}

void app_main(void)
{
    esp_err_t ret;
    init_process();  
    gdispf=false;
    #ifdef DISPLAY
        ret=init_lcd();
        if(ret==ESP_OK)
        {
            xTaskCreate(&displayManager,"dMgr",1024*4,NULL, 5, NULL); 	       
            gdispf=true;
        } 
    #endif

    theMeter.initMeter(ssignal);        //load the meter and all infra required like fram/pcnt etc

    if(theConf.meterconf==0  )  // is this meter configured???
    {
        //no, start the configuration phase
        //need to format fram to assure that new frrams are not with junk
        ESP_LOGW(MESH_TAG,"Formatting Fram new configuration");
        theMeter.deinit();
        theMeter.format();
        xTaskCreate(&blinkConf,"displ",1024,NULL, 5, &configureHandle); 	        //blink we are not configured
        reconfTimer=xTimerCreate("Reconf",pdMS_TO_TICKS(300000),pdFALSE,( void * ) 0, [] ( TimerHandle_t xTimer){esp_restart();});   //monitor activity and tiemout if no work done-> use lambda
        meter_configure();      //start the STA for access to Router directly.
                                    // need the Router to send MQTT to HQ with Challenge ID(CID) and Passwiord(PID)
                                    // but if NRT, we cannot use this. Need to connect mesh and send a meshmsg with same
                                    // info as MQTT, but mesh root will send it for us
        vTaskDelete(configureHandle);   //stop the blinking now we are configured
    }

// todo refactor all this
    xTaskCreate(&root_timer,"reptimer",1024*8,NULL, 5, NULL); 	        
// the internal mesh is now going to start and begin all the main flow from its gotIp event manager
    start_mesh();
    
    ESP_LOGI(MESH_TAG,"Heap Free APP %d",esp_get_free_heap_size());
       

}
//dev