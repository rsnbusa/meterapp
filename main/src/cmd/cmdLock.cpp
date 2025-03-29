#define GLOBAL
#include "globals.h"
extern void writeLog(char *que);
int cmdLock(void *argument)
{
    cJSON               *root,*cualm,*lstate,*elcmd;
    mesh_data_t         data;
    int                 err;
    mqttSender_t        meshMsg;

    /*
    {"cmd":"lock","disCon":[{"mid:"meterid","state":0/1},{"mid":"othermid","state":01/}...]}
    */
    elcmd=(cJSON *)argument;
    if(elcmd)
    {
            cJSON *monton= 		    cJSON_GetObjectItem(elcmd,"disCon");
            if(monton)
            {

                int son=cJSON_GetArraySize(monton);
                for (int a=0;a<son;a++)
                {
                    cJSON *cmdIteml 	=cJSON_GetArrayItem(monton, a);//next item
                    if(cmdIteml)
                    {
                        cJSON *cualm			=cJSON_GetObjectItem(cmdIteml,"mid"); //get meter id
                        cJSON *lstate		    =cJSON_GetObjectItem(cmdIteml,"state"); //get lock state
                        if(cualm && lstate)
                        {
                            root=cJSON_CreateObject();
                            if(root)
                            {
                                cJSON_AddStringToObject(root,"cmd",internal_cmds[4]);
                                cJSON_AddStringToObject(root,"mid",cualm->valuestring);
                                cJSON_AddNumberToObject(root,"state",lstate->valueint);

                                char *intmsg=cJSON_PrintUnformatted(root);
                                if(intmsg)
                                {  
                                    char *aca=(char*)malloc(100);
                                    sprintf(aca,"Lock Mid %s",cualm->valuestring);
                                    writeLog(aca);
                                    free(aca);
                                    data.data   =(uint8_t*)intmsg;
                                    data.size   =strlen(intmsg);
                                    data.proto  = MESH_PROTO_BIN;
                                    data.tos    = MESH_TOS_P2P;

                                //need to send a broadcast message to all stations so they can lock
                                // this ID meter to whatever state was sent
                                    err= esp_mesh_send( &GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP); 
                                    if(err)
                                        ESP_LOGE(MESH_TAG,"Lock Broadcast failed %x",err);
                                    free(intmsg); 
                                }
                                else 
                                    ESP_LOGE(MESH_TAG,"Lock fail to get ram for msg");
                                cJSON_Delete(root); 
                            }
                            else 
                            {
                                ESP_LOGE(MESH_TAG,"Error creating Root Lock");
                                return ESP_FAIL;
                            }
                        }
                        else 
                        {
                            ESP_LOGE(MESH_TAG,"Lock Cmd no MID or STATE");
                            return ESP_FAIL;
                        }
                    }
                    else
                    {
                            ESP_LOGE(MESH_TAG,"Lock Cmd no Array error");
                            return ESP_FAIL;
                    }
                }
            }
            else
            {
                ESP_LOGE(MESH_TAG,"Lock Cmd no disCon array");
                return ESP_FAIL;
            }
        return ESP_OK;
    }
    else 
    {
       ESP_LOGE(MESH_TAG,"NO CMD argument passed. Internal error");
    }
    return ESP_FAIL;
}
