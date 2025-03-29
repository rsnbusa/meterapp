/*
 * webserver.cpp
 *
 *  Created on: Dec 29, 2019
 *      Author: rsn
 */
#define GLOBAL

#include "includes.h"
#include "globals.h"

extern int aes_encrypt(const char* src, size_t son, char *dst,const char *cualKey);

static esp_err_t conn_base(httpd_req_t *req);
static esp_err_t configure(httpd_req_t *req);
static esp_err_t cancel(httpd_req_t *req);

extern const unsigned char params_start[] 		asm("_binary_index_min_html_start");	//default base html
extern const unsigned char ok_start[] 			asm("_binary_ok_png_start");
extern const unsigned char ok_end[] 			asm("_binary_ok_png_end");
extern const unsigned char nak_start[] 			asm("_binary_nak_png_start");
extern const unsigned char nak_end[] 			asm("_binary_nak_png_end");
extern const unsigned char fav_start[] 			asm("_binary_favicon_ico_start");
extern const unsigned char fav_end[] 			asm("_binary_favicon_ico_end");
extern const unsigned char cher_start[] 		asm("_binary_cher_png_start");
extern const unsigned char cher_end[] 			asm("_binary_cher_png_end");
extern const unsigned char cancel_start[] 		asm("_binary_cancel_png_start");
extern const unsigned char cancel_end[] 		asm("_binary_cancel_png_end");

extern void delay(uint32_t a);
extern void write_to_flash();
extern uint32_t xmillis();

// int wifi_bytes=wifi_end-wifi_start;
// int check_bytes=check_end-check_start;
// int nocheck_bytes=nocheck_end-nocheck_start;
int ok_bytes=ok_end-ok_start;
int nak_bytes=nak_end-nak_start;
int fav_bytes=fav_end-fav_start;
int cher_bytes=cher_end-cher_start;
int cancel_bytes=cancel_end-cancel_start;

typedef struct httpd_uri ur;
#define MAXURLS 8
ur urls[MAXURLS];

bool getParam(char *buf,char *cualp,char *donde)
{
	if( httpd_query_key_value(buf, cualp, donde, 30)==ESP_OK)
		return true;
	else
		return false;
}

esp_err_t sendcancel(httpd_req_t *req)
{
	printf("Send cancel icon\n");
	httpd_resp_set_hdr(req,"Cache-Control","private, max-age=86400");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)cancel_start,cancel_bytes);
	return ESP_OK;
}

esp_err_t sendcher(httpd_req_t *req)
{
	printf("Cherry get\n");
	httpd_resp_set_hdr(req,"Cache-Control","private, max-age=86400");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)cher_start,cher_bytes);
	return ESP_OK;
}

esp_err_t sendfav(httpd_req_t *req)
{
	httpd_resp_set_hdr(req,"Cache-Control","private, max-age=86400");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)fav_start,fav_bytes);
	return ESP_OK;
}

esp_err_t sendok(httpd_req_t *req)
{
	httpd_resp_set_hdr(req,"Cache-Control","private, max-age=86400");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)ok_start,ok_bytes);
	return ESP_OK;
}

esp_err_t sendnak(httpd_req_t *req)
{
	httpd_resp_set_hdr(req,"Cache-Control","private, max-age=86400");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)nak_start,nak_bytes);

	return ESP_OK;
}
esp_err_t cancel(httpd_req_t *req)
{
	printf("Cancel called\n");
	httpd_resp_set_hdr(req,"Cache-Control","public, no-cache");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)cancel_start,cancel_bytes);
	theConf.meterconf=1;
	write_to_flash();
	esp_restart();
	return ESP_OK;
}

void setHeaders(httpd_req_t *req,char * tipo)
{
	httpd_resp_set_hdr(req,"Cache-Control","public, no-cache");
	httpd_resp_set_hdr(req,"Access-Control-Allow-Credentials","true");
	httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
	httpd_resp_set_type(req,tipo);
}


void init_urls()
{
	urls[0].uri       = "/";				urls[0].handler   = conn_base;
	urls[1].uri       = "/configure";		urls[1].handler   = configure;
	urls[2].uri       = "/ok.png";			urls[2].handler   = sendok;
    urls[3].uri       = "/nak.png";			urls[3].handler   = sendnak;
    urls[4].uri       = "/favicon.ico";		urls[4].handler   = sendfav;
    urls[5].uri       = "/cher.png";		urls[5].handler   = sendcher;
    urls[6].uri       = "/cancel";			urls[6].handler   = cancel;
    urls[7].uri       = "/cancel.png";		urls[7].handler   = sendcancel;
}



static esp_err_t http_event_handle(esp_http_client_event_t *evt)
{
	char *aqui;

    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
			if (!esp_http_client_is_chunked_response(evt->client))
			{
				aqui=(char*)evt->user_data;
				memcpy(aqui,evt->data,evt->data_len);
				aqui[evt->data_len]=0;
			}
            break;
        default:
        	break;
    }
    return ESP_OK;
}

void getSetParameter(char *buf,char * cual, int llimit,int hlimit,int def,int *donde)
{
	char param[60];
	int valor;

	if(getParam(buf,cual,param))
	{
		valor=atoi(param);
		if(valor<llimit)
			valor=llimit;
		if(valor>hlimit)
		{
			valor=hlimit;
		}
	}
	else	// parameter not passed use default
	{
		printf("Could not find %s using default\n",cual);
		valor=def; //default
	}
	*donde=valor;
}

void getSetParameterFloat(char *buf,char * cual, float llimit,float hlimit,float def,float *donde)
{
	char param[60];
	float valor;

	if(getParam(buf,cual,param))
	{
		valor=atof(param);
		if(valor<llimit)
			valor=llimit;
		if(valor>hlimit)
		{
			valor=hlimit;
		}
	}
	else	// parameter not passed use default
	{
		printf("Could not find %s using float default\n",cual);
		valor=def; //default
	}
	*donde=valor;
}

int save_sta_pass(char *sta,char * psw)
{
	    wifi_config_t       configsta,configsta2;
		int err; 

	    err=esp_wifi_get_config( WIFI_IF_STA,&configsta);      // get station ssid and password
		if(!err)
		{
			//   printf("Configure Sta %s Pasw %d\n",configsta.sta.ssid,configsta.sta.password);

				ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
				bzero(&configsta.sta.ssid,sizeof(configsta.sta.ssid));
				bzero(&configsta.sta.password,sizeof(configsta.sta.password));
				memcpy(&configsta.sta.ssid,sta,strlen(sta));
				memcpy(&configsta.sta.password,psw,strlen(psw));
				err=esp_wifi_set_config( WIFI_IF_STA,&configsta);      // save new ssid and password
				if(err)
				{
					printf("Failed to save new ssid %x\n",err);
					return ESP_FAIL;
				}
					    err=esp_wifi_get_config( WIFI_IF_STA,&configsta2);      // get station ssid and password
							//   printf("Post Configure Sta %s Pasw %d\n",configsta2.sta.ssid,configsta2.sta.password);


		}
		else
		{
			printf("Could not get STA config update ssid %x\n", err);       
			return ESP_FAIL;
		}
		return ESP_OK;
}

bool check_key(uint64_t key)
{
	char kkey[17],laclave[33];
	int challenge;
	uint8_t *porg,*pdest;

	ESP_LOG_BUFFER_HEX(MESH_TAG,&key,4);

	porg=(uint8_t*)&key;
	pdest=(uint8_t*)&challenge;
	pdest+=3;

	for (int a=0;a<4;a++)
	{
		*pdest=*porg;
		pdest--;
		porg++;
	}

	sprintf(kkey,"%016d",theConf.cid);
	// printf("num [%s]\n",kkey);
	sprintf(laclave,"%s%s",kkey,kkey);
	// printf("clave [%s] %d\n",laclave,strlen(laclave));
	char *aca=(char*)calloc (100,1);

	int ret=aes_encrypt(SUPERSECRET,sizeof(SUPERSECRET),aca,laclave);
	if(ret==ESP_FAIL)
	{
		ESP_LOGE(MESH_TAG,"Fail encrypt");
		return false;
	}

	ESP_LOG_BUFFER_HEX(MESH_TAG,aca,strlen(SUPERSECRET));
	ESP_LOG_BUFFER_HEX(MESH_TAG,&challenge,4);

int como=memcmp((void*)aca,(void*)&challenge,4);
// printf("Como %d\n",como);
	free(aca);
	if(como==0)
		return true;

	return false;

}

void save_inst_msg(char *mid, int bpk,int kwhstart,char *who)
{
	time_t	ts;
	uint8_t     thismac[6];
	uint32_t imac;


	time(&ts);

    esp_base_mac_addr_get((uint8_t*)thismac);
	memcpy(&imac,&thismac[2],4);;

    cJSON *local_root=cJSON_CreateObject();
    if(local_root==NULL)
    {
        printf("cannot create root installmsg\n");
        return;
    }

	cJSON_AddStringToObject(local_root,"cmd","install");
	cJSON_AddStringToObject(local_root,"mid",mid);
    cJSON_AddNumberToObject(local_root,"mac",imac);
    cJSON_AddNumberToObject(local_root,"bpk",bpk);
    cJSON_AddNumberToObject(local_root,"kwhs",kwhstart);
    cJSON_AddNumberToObject(local_root,"nodeid",theConf.controllerid);
    cJSON_AddNumberToObject(local_root,"subnode",theConf.subnode);
    cJSON_AddNumberToObject(local_root,"prov",theConf.provincia);
    cJSON_AddNumberToObject(local_root,"cant",theConf.canton);
    cJSON_AddNumberToObject(local_root,"parro",theConf.parroquia);
    cJSON_AddNumberToObject(local_root,"cp",theConf.codpostal);
	cJSON_AddStringToObject(local_root,"who",who);
	cJSON_AddNumberToObject(local_root,"forced",0);
    cJSON_AddNumberToObject(local_root,"ts",(uint32_t)ts);
	char *lmessage=cJSON_PrintUnformatted(local_root);
	if(lmessage)
	{
		bzero(theConf.instMsg,sizeof(theConf.instMsg));
		strcpy(theConf.instMsg,lmessage);
		theConf.instMsglen=strlen(lmessage);
		// printf("Inst message [%s] %d\n",theConf.instMsg,theConf.instMsglen);
		write_to_flash();
		free(lmessage);
	}
	else
		ESP_LOGE(MESH_TAG,"No RAM for cjson installmsg");

	cJSON_Delete(local_root);

}
static esp_err_t configure(httpd_req_t *req)
{
	char temp[20],param[40],laclave[20],elmesh[20],meshpsw[20],usuario[20];
	int desde=0,hasta,buf_len;
	char *buf=NULL;
	char *answer;
	bool errores=false;
	char *ptr;
	uint32_t keyread;
	time_t	now;

// printf("Configuration cid\n");
	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1)
	{
		buf = (char*)calloc(buf_len,1);
		if(buf)
		{
			if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
			{
				bzero(param,sizeof(param));
				if(getParam(buf,(char*)"passw",param))
				{
					strcpy(laclave,param);
					// ESP_LOG_BUFFER_HEX(MESH_TAG,&param,strlen(param));
					keyread=strtoul(laclave, &ptr, 16);
					// printf("Key %u %d\n",keyread,theConf.cid);
					ESP_LOG_BUFFER_HEX(MESH_TAG,&keyread,sizeof(keyread));

					if(!check_key(keyread))
					{
						// printf("Not same key %d %d\n",keyread,theConf.cid);
						sendnak(req);
						return ESP_OK;
					}
					if(getParam(buf,(char*)"who",param))
					{
						strcpy(usuario,param);
						if(strlen(usuario)>8)
							ESP_LOGI(MESH_TAG,"Web Station User Updated");
						else 
						{
							ESP_LOGI(MESH_TAG,"Failed Update User");
							sendnak(req);
							return ESP_OK;
						}
					}
					if(getParam(buf,(char*)"meshsta",param))
					{
						strcpy(elmesh,param);
						if(strlen(elmesh)>0)
						{
							if(getParam(buf,(char*)"meshpsw",param))
							{
								strcpy(meshpsw,param);
								if(strlen(meshpsw)>7)	//minimum 8 chars for wp2
								{
									if(save_sta_pass(elmesh,meshpsw)==ESP_OK)
										ESP_LOGI(MESH_TAG,"Web Station/Password Updated");
									else 
										ESP_LOGI(MESH_TAG,"Failed Update Sta/Password");
								}
							}
						}
					}
				}

				errores=false;
				write_to_flash();
				//mesh and network paramerters
				if(!theConf.ptch)
				{
					bzero(&theConf.instMsg,sizeof(theConf.instMsg));
					getSetParameter(buf,"nodeid",1,100000,1,(int*)&theConf.controllerid);
					getSetParameter(buf,"slot",0,1799,100,(int*)&theConf.mqttSlots);
					getSetParameter(buf,"prov",1,200,1,(int*)&theConf.provincia);
					getSetParameter(buf,"cant",1,500,1,(int*)&theConf.canton);
					getSetParameter(buf,"parro",1,500,1,(int*)&theConf.parroquia);
					getSetParameter(buf,"codp",1,9999999,1,(int*)&theConf.codpostal);
				}

					if(!medidorlock)	//its locked, skip 
					{						
						// bzero((uint8_t*)&medidor,sizeof(meterType));
						if(!getParam(buf,"m1",(char*)&theConf.medback.mid))
						{
							ESP_LOGI(MESH_TAG,"Name error meter %s",temp);
							errores=true;
						}
						theMeter.setMID(theConf.medback.mid);
						if(strlen(theConf.medback.mid)!=0)
						{	
							getSetParameter(buf,"b1",100,3000,800,(int*)&theConf.medback.bpk);
							theMeter.setBPK(theConf.medback.bpk);
							getSetParameter(buf,"s1",0,3000000,0,(int*)&theConf.medback.kwhstart);
							theMeter.setKstart(theConf.medback.kwhstart);
							theMeter.setLkwh(theConf.medback.kwhstart);
							uint8_t pp;
							getSetParameter(buf,"p1",0,1,0,(int*)&pp);
							theMeter.setPay(pp);
						}
					}
									
				
				if(errores)
				{
					sendnak(req);
					ESP_LOGI(MESH_TAG,"Configuration aborted");
				}
				else
				{
					sendok(req);
					time(&now);
					// save data to flash and Fram
					theConf.meterconf=1; // configuration is done
					theConf.ptch=1;		//Pop that Cherry :) no longer a virgin chip now a happy donout
					theConf.meterconfdate=now;	//configuration date
					theConf.bornDate=theConf.meterconfdate;
					theConf.cid=0;				//useless now
					theConf.medback.backdate=now;
					write_to_flash();

					if(!(medidorlock || strlen(theConf.medback.mid)==0))
					{
						theMeter.setLkwh(theMeter.getKstart());
						theMeter.saveConfig();
					}
					save_inst_msg(theMeter.getMID(),theMeter.getBPK(),theMeter.getKstart(),usuario);
					ESP_LOGI(MESH_TAG,"Configuration done");
				}
				ESP_LOGI(MESH_TAG,"Restarting");
				delay(1000);
				esp_restart();
			}	
			free(buf);				
		}
	}
	return ESP_OK;
}

int sendHtmlInt(char *que, char * params, char *answer)
{
	char	textl[100];

	esp_http_client_config_t lconfig;

	memset(&lconfig,0,sizeof(lconfig));
	sprintf(textl,"%s",que);		//ap of esp32
	// sprintf(textl,"https://www.meteriot.site/%s",que);
	lconfig.url=textl;
	lconfig.user_data=(void*)answer;
	lconfig.cert_pem = NULL;
	// lconfig.cert_pem = (char *)server_cert_pem_start;
	lconfig.skip_cert_common_name_check = true;

	#ifdef DEBUGX
		if(theConf.traceflag & (1<<WEBD))
			pprintf("%sSending HTML %s params %s\n",HOSTDT,textl,params);
	#endif

		lconfig.event_handler = http_event_handle;							//in charge of saving received data to Fram directly

		esp_http_client_handle_t client = esp_http_client_init(&lconfig);
		if(client)
		{
			if(strlen(params))
			{
		    	esp_http_client_set_method(client, HTTP_METHOD_POST);
			    esp_http_client_set_post_field(client, params, strlen(params));
			}
			esp_err_t err = esp_http_client_perform(client);			// do the hard work
			if (err == ESP_OK)
			{
	#ifdef DEBUGX
				if(theConf.traceflag & (1<<HOSTD))
					pprintf("%sStatus = %d, content_length = %d\n",HOSTDT,
						esp_http_client_get_status_code(client),
						esp_http_client_get_content_length(client));
	#endif
				if(esp_http_client_get_status_code(client)!=200)
				{
					// if(theConf.traceflag & (1<<HOSTD))
						ESP_LOGI(MESH_TAG,"Failed to send HTML %x",esp_http_client_get_status_code(client));
					esp_http_client_cleanup(client);
					return ESP_FAIL;
				}
			}
			else
			{
#ifdef DEBUGX
				if(theConf.traceflag & (1<<HOSTD))
					pprintf("%sFailed to send HTML %x\n",HOSTDT,err);
				esp_http_client_cleanup(client);
				return ESP_FAIL;
#endif
			}
			// all is well, cleanup and read back to our working array
			esp_http_client_cleanup(client);
			return ESP_OK;
		}
		else
		{
			ESP_LOGI(MESH_TAG,"Failed to create HTTP CLient");
			return ESP_FAIL;
		}
		return ESP_OK;

}

int sendHtml(httpd_req_t *req,const char * format, ...)
{
	va_list args;
	char *ltemp;

	ltemp=(char*)calloc(7000,1);
	if(!ltemp)
		return ESP_FAIL;
	// else
	// 	bzero(ltemp,7000);

	va_start (args, format);
	vsprintf (ltemp,format, args);
	va_end (args);

    if(strlen(ltemp)<=7000)
	    httpd_resp_send(req,ltemp, strlen(ltemp));
	FREEANDNULL(ltemp);
	return ESP_OK;
}

void wmonitorCallback( TimerHandle_t xTimer )
{
	webState=wNONE;
	ESP_LOGI(MESH_TAG,"TimeOut Webserver");
	esp_restart();
}

static esp_err_t conn_base(httpd_req_t *req)		//default page
{
	char texto[100],dis[10];

	setHeaders(req,(char*)"text/html");

	webLogin=false;			//Reset login state
	bzero(tempb,7000);

	// if(!theConf.ptch && theConf.meshconf<2)
	// {
	// 	theConf.meshid=esp_random() % 9999999;
	// 	ESP_LOGI(MESH_TAG,"Mesh Id new %8X",theConf.meshid);
	// }
	
	if(theConf.ptch)
		strcpy(dis,"disabled");
	else
		strcpy(dis," ");

	if(sendHtml(req,(char*)params_start,theConf.ptch?0:40,theConf.cid,theConf.controllerid,dis,theConf.mqttSlots,dis,theConf.provincia,dis,theConf.canton,dis,theConf.parroquia,dis,theConf.codpostal,dis,
	theMeter.getMID(),medidorlock?"disabled":"",theMeter.getBPK(),medidorlock?"disabled":"",theMeter.getKstart(),
	medidorlock?"disabled":"",theMeter.getLkwh(),theMeter.getPay()
	)!=0)
		ESP_LOGI(MESH_TAG,"Failed to send html");

	webState=wNONE;
	xTimerStop(webTimer,0); //Stop it in case we are coming back
	xTimerStart(webTimer,0); //Start it
	return ESP_OK;
}

void urldecode2(char *dst, const char *src)
{
        char a, b;
        while (*src) {
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
        }
        *dst++ = '\0';
}


void start_webserver()
{
	ESP_LOGW(MESH_TAG,"Stating webserver");

	bzero(&io_conf,sizeof(io_conf));
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<BEATPIN);
    gpio_set_level((gpio_num_t)BEATPIN,1);
    gpio_config(&io_conf);

	tempb=(char*)calloc(7000,1);
	if(!tempb)
	{
		ESP_LOGI(MESH_TAG,"No ram");
		vTaskDelete(NULL);
	}
	#ifndef SECURE
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_uri_handlers=MAXURLS;
	config.max_open_sockets=1;		//just one connection
	config.stack_size=14000;
	int ret=httpd_start(&wserver, &config);	//global wserver to stop it if Firmware called
#else
	httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
	config.httpd.max_uri_handlers=MAXURLS;
	config.httpd.max_open_sockets=4;		//just one connection
	config.httpd.stack_size=14000;

	extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
	extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
	config.cacert_pem = cacert_pem_start;
	config.cacert_len = cacert_pem_end - cacert_pem_start;
	extern const unsigned char pprvtkey_pem_start[] asm("_binary_wprvtkey_pem_start");
	extern const unsigned char pprvtkey_pem_end[]   asm("_binary_wprvtkey_pem_end");
	config.prvtkey_pem = pprvtkey_pem_start;
	config.prvtkey_len = pprvtkey_pem_end - pprvtkey_pem_start;

   int ret=httpd_ssl_start(&wserver, &config);	//global wserver to stop it if Firmware called
   #endif
//    printf("Webstart code %x\n",ret);
   if(ret== ESP_OK)
    {
	   #ifdef DEBUGX
	      if(theConf.traceflag & (1<<WEBD))
		  #ifdef SECURE
	      	pprintf("%sSSL server started on port:%d\n",WEBDT, config.httpd.server_port);
			  #else
	       	 pprintf("%sServer started on port:%d\n",WEBDT, config.server_port);
				#endif
	   #endif
	   init_urls();

		webState=wNONE;
		webLogin=true;
        webTimer=xTimerCreate("Monitor",300000 /portTICK_PERIOD_MS,pdTRUE,NULL,&wmonitorCallback);	//5 minutes if no activity back to wNONE

    	bzero(gwStr,sizeof(gwStr));
		
        // Set URI handlers
    	for(int a=0;a<MAXURLS;a++)
    	{
    		// printf("url %s\n",urls[a].uri);
    		urls[a].method=HTTP_GET;
    		urls[a].user_ctx=NULL;
    		ret=httpd_register_uri_handler(wserver, &urls[a]);
			if(ret)
				ESP_LOGE(MESH_TAG,"Error setting URL %x for url %d",ret,a);
    	}

#ifdef DEBUGX
    if(theConf.traceflag & (1<<WEBD))
    	pprintf("%sSSL WebServer Started\n",WEBDT);
#endif
    }
    else
    	ESP_LOGE(MESH_TAG,"Could not start webserver %x %s",ret,esp_err_to_name(ret));
	// ESP_LOGI(MESH_TAG,"Web die");
    // vTaskDelete(NULL);
}


