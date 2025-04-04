#ifndef MAIN_GLOBALS_H
#define MAIN_GLOBALS_H

#ifdef GLOBAL
    #define EXTERN extern
#else
    #define EXTERN
#endif

#include "typedef.h"    
#include "defines.h"
const static int MQTT_BIT 				= BIT0;
const static int WIFI_BIT 				= BIT1;
const static int PUB_BIT 				= BIT2;
const static int DONE_BIT 				= BIT3;
const static int SNTP_BIT 				= BIT4;
const static int SENDMQTT_BIT			= BIT5;
const static int SENDH_BIT 				= BIT6;
const static int DISCO_BIT 				= BIT7;
const static int ERROR_BIT 				= BIT8;
const static int ERRDATE_BIT 			= BIT9;
// for othergroup
const static int REPEAT_BIT 				= BIT0;
const static int MESHRX_BIT 				= BIT1;
const static int TIMER2_BIT 				= BIT2;


EXTERN TaskHandle_t                 recoTaskHandle,mqttSendHandle,mqttMgrHandle,showHandle,configureHandle,lvHandle;
EXTERN esp_aes_context		        actx ;
EXTERN uint8_t                      mqttErrors,ssignal,dia,hora,mes,MESH_ID[6],lastline;
EXTERN int16_t                      theGuard,timeSlotStart,timeSlotEnd,sentMqtt,meterCount;
EXTERN int                          lastheap,acumheap,s_retry_num,mesh_layer,collectTimer,BASETIMER;
EXTERN bool                         donef,mqttf,meshf,webLogin,mesh_started,nakf,logof,okf,favf,framFlag,sendMeterf,
                                    medidorlock,hostflag,loadedf,firstheap,loginf,gdispf;
EXTERN SemaphoreHandle_t 		    flashSem,tableSem;
EXTERN framArgs_t                   framArg;
EXTERN esp_mqtt_client_config_t 	mqtt_cfg;
EXTERN esp_mqtt_client_handle_t     clientCloud;
EXTERN QueueHandle_t 				mqttQ,mqttR,mqttSender,mqtt911,meshQueue; 
EXTERN cmdRecord 					cmds[MAXCMDS];
EXTERN config_flash                 theConf;
EXTERN nvs_handle 					nvshandle;
EXTERN EventGroupHandle_t 			wifi_event_group,s_wifi_event_group,otherGroup;
EXTERN TimerHandle_t                dispTimer,beatTimer,firstTimer,repeatTimer,webTimer,sendMeterTimer,reconfTimer,recconectTimer,confirmTimer;
EXTERN mesh_addr_t                  s_route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
EXTERN int                          s_route_table_size,counting_nodes,msgout;
//kbd stuff
EXTERN loglevel_t                   loglevel;
EXTERN timerl_t                     basetimer;
EXTERN prepaid_t                    prepaidcmd;
EXTERN resetconf_t                  resetlevel;
EXTERN securit_t                    kbdsedcurity,appSSID,appNode; 
EXTERN logargs_t                    logArgs;
EXTERN adc_t                        adcArgs;
// end kbd 
EXTERN aes_en_dec_t                 endec;
EXTERN esp_netif_t*                 esp_sta; 
EXTERN mesh_addr_t                  mesh_parent_addr;    
EXTERN esp_ip4_addr_t               s_current_ip;       
EXTERN httpd_handle_t 				wserver;
EXTERN wstate_t						webState;
EXTERN char                         gwStr[20],*tempb,iv[16],key[32],cmdQueue[60],infoQueue[60],internal_cmds[MAXINTCMDS][20],
                                    emergencyQueue[60],cmdBroadcast[60],discoQueue[60],installQueue[60],*globalDupStr;
EXTERN mesh_addr_t                  GroupID; 
EXTERN master_node_t                masterNode;
EXTERN esp_console_cmd_t            node_cmd,ota_cmd,fram_cmd,meter_cmd,mid_cmd,config_cmd,erase_cmd,loglevel_cmd,basetimer_cmd,prepaid_cmd,resetconf_cmd,
                                    aes_cmd,security_cmd,app_cmd,log_cmd,adc_cmd,findunit_cmd,meshreset_cmd;
EXTERN gpio_config_t 	            io_conf;
EXTERN FILE*                        myFile;
EXTERN TaskHandle_t                 adc_task_handle,blinkHandle,ssidHandle;
EXTERN int                          lastKnowCount,theAddress,wmeter;
EXTERN esp_console_repl_t           *repl;
EXTERN esp_console_repl_config_t    repl_config ;
EXTERN char                         apssid[32],appsw[10];
EXTERN uint32_t                     gCRC,meterSize;
EXTERN meshp_t                      param;
EXTERN meterClass                   theMeter;
EXTERN lv_disp_t                   *disp;
EXTERN esp_lcd_panel_handle_t       panel_handle;
EXTERN esp_lcd_panel_io_handle_t    io_handle;
EXTERN esp_timer_handle_t           lvgl_tick_timer;
EXTERN lv_display_t                 *display;
EXTERN void                         *dispbuf;
#endif
