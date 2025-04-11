#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

void showconf(void *pArg)
{

  char            buf[50],buf2[50],fecha[60],myssid[20];
  time_t          lastwrite,now;
  struct tm       timeinfo;
  // portMUX_TYPE    xTimerLock = portMUX_INITIALIZER_UNLOCKED;
  TickType_t      xRemainingTime;
  int             routet;
  mesh_type_t     typ;
  char            my_mac[8]={0};
  uint8_t         min,secs;
  mesh_addr_t     bssid;
  time_t         guardDate,bootdate;

  time(&now);
  localtime_r(&now, &timeinfo);

  bzero(myssid,sizeof(myssid));
  wifi_config_t conf;
  if(esp_wifi_get_config(WIFI_IF_STA, &conf)!=ESP_OK)
  {
    printf("Error readinmg wifi config\n");
    strcpy(myssid,(char*)conf.sta.ssid);
  }

  typ=esp_mesh_get_type();

  char *tipo[]={"Idle","ROOT","NODE","LEAF","STA"};

  unsigned char mac_base[6] = {0};
  esp_efuse_mac_get_default(mac_base);
  esp_read_mac(mac_base, ESP_MAC_WIFI_STA);

  if(mesh_started)
      memcpy(my_mac,mesh_netif_get_station_mac(),6);

  min=theConf.mqttSlots/30;
  secs=theConf.mqttSlots*2-(min*60);

  timeinfo.tm_min=min;
  timeinfo.tm_sec=secs; 
  time_t nexthour = mktime(&timeinfo);
  int faltan=nexthour-now;
  int fhora=faltan/3600;
  int fmin=faltan/60;
  int fsecs=faltan-(fmin*60);

 
  esp_mesh_get_parent_bssid(&bssid);
  const esp_app_desc_t *mip=esp_app_get_description();
if(mip)
  printf("\t\t Mesh Configuration Date %s ",ctime(&now));

  if(!theConf.ptch)
      printf("Virgin Chip\n");

uint32_t nada;    // this is compiler error, it goes crazy if done directly like fram.read_fdate(uint8_t*)&guarddate)

    nada=theMeter.getLastUpdate();
    // fram.read_fdate((uint8_t*)&nada);				// read last saved datetime.
    guardDate=(time_t)nada;

  bootdate=(time_t)theConf.lastRebootTime;    //same compiler error

  printf("\n\t\t\t Device Stuff\n");
  printf("\t\t\t ============\n");
  printf("BootCount:%d LastReset:%d Reason:%d LogLevel:%d DownTime: %lus LastReboot %s",theConf.bootcount,theConf.lastResetCode,theConf.lastResetCode,
            theConf.loglevel,theConf.downtime,ctime((time_t*)&bootdate));
  printf("MeterConf %d Mqttf %d sendMeterf %d\n",theConf.meterconf,mqttf,sendMeterf);
  printf("Guard Date %s",ctime(&guardDate));
  printf("Securty Check: %s\n",theConf.useSec?"Yes":"No");
  printf("Display Active: %s\n",gdispf?"Yes":"No");
  printf("App Version: %s IDF: %s Project: %s\nCompile Date %s & %s\n",mip->version,mip->idf_ver,mip->project_name,mip->date,mip->time);
  printf("OTA Url [%s]\n",theConf.OTAURL);

  printf("\n\t\t\t Backup Stuff\n");
  printf("\t\t\t ============\n");
  printf("MID:%s\tBPK:%d\tkwhStart:%d\tDate:%s",theConf.medback.mid,theConf.medback.bpk,theConf.medback.kwhstart,ctime(&theConf.medback.backdate));
  
  printf("\n\t\t\t Network Stuff\n");
  printf("\t\t\t =============\n");
  printf("Mesh Id:"MACSTR" SubNode:%d NodeID:%d\n",MAC2STR(MESH_ID),theConf.subnode,theConf.controllerid);
  printf("[Sta:[%s]-Psw:[%s]] ",conf.sta.ssid,conf.sta.password);
  printf("[FlashConfig_Sta:[%s] & Psw:[%s]]\n",theConf.thessid,theConf.thepass);
  printf("Parent BSSID " MACSTR "\n",MAC2STR(bssid.addr));  
  esp_wifi_get_config(WIFI_IF_AP, &conf);
  printf("AP:[%s]-Pswd:[%s]\n",conf.ap.ssid,conf.ap.password);

  printf("\n\t\t\t Mqtt Topics\n");
  printf("\t\t\t ===========\n");
  printf("Command Topic \t\t%s\n",cmdQueue);
  printf("Info Topic \t\t%s\n",infoQueue);
  printf("DisCon Topic \t\t%s\n",discoQueue);
  printf("Install Topic \t\t%s\n",installQueue);
  printf("Mqtt server [%s] pass [%s] user [%s] Skips [%d]\n",theConf.mqttServer,theConf.mqttPass,theConf.mqttUser,theConf.maxSkips);

  printf("\n\t\t\t FRAM Stuff\n");
  printf("\t\t\t ============\n");
  printf("Lost Pulses %d\tFramWrites %d\tFramReads %d\n",theMeter.getLost_Pulses(),theMeter.getFram_Writes(),theMeter.getFram_Reads());
  printf("\n\t\t\t Timing Stuff\n");
  printf("\t\t\t ============\n");
  if(esp_mesh_is_root())
  {
    if(repeatTimer)
      if(xTimerIsTimerActive(repeatTimer))
      {
        xRemainingTime = xTimerGetExpiryTime( repeatTimer ) - xTaskGetTickCount();
      }
    printf("Next SendData %dms BaseTime %d Repeat Timer %d\n",xRemainingTime,theConf.baset,theConf.repeat);

  }
    printf("Slot:%d @ %02d:%02d Created:%s",theConf.mqttSlots,min,secs,ctime(&theConf.bornDate));

  mesh_addr_t mmeshid;
  esp_mesh_get_id(&mmeshid);
  printf("\n\t\t\t MESH Stuff\n");
  printf("\t\t\t ==========\n");
  printf("NodeType:%s MAC:" MACSTR ", MeshID<"MACSTR">\n", tipo[typ],MAC2STR(mac_base),MAC2STR(mmeshid.addr));
  printf("Device state %s\n",esp_mesh_is_device_active?"Up":"Down");

  // is mesh active?
  if(meshf)
  {
      esp_err_t err=esp_mesh_get_routing_table((mesh_addr_t *) &s_route_table,CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &routet);

      if(err==ESP_OK)
      {
        printf("Mesh Network\n");
        if(routet<11)
        {
          for (int a=0;a<routet;a++)
            printf("\tMAC[%d]:" MACSTR " %s MeterId [%s] Send [%s] PWR[%s] Skip [%s] Skipped [%d/%d]\n",a,MAC2STR(s_route_table[a].addr),MAC_ADDR_EQUAL(s_route_table[a].addr, my_mac)?"ME":"",masterNode.theTable.meterName[a],
                masterNode.theTable.sendit[a]?"Y":"N",masterNode.theTable.onoff[a]?"Off":"On",theConf.allowSkips?"Yes":"No",masterNode.theTable.skipcounter[a],theConf.maxSkips);
        }
        else
          printf("There are %d nodes\n",routet);
      }
  }
    time_t  ahora=theMeter.getStatsLastCountTS();
    printf("\n\t\t\t Statistics\n");
    printf("\t\t\t ===============\n");
    printf("BytesOut:%d\tBytesIn:%d\tMsgIn:%d\t\tMsgOut:%d\n",theMeter.getStatsBytesOut(),theMeter.getStatsBytesIn(),theMeter.getStatsMsgIn(),theMeter.getStatsMsgOut());
    printf("MeterCount: %d\tNodeCount: %d \n",theMeter.getStatsLastMeterCount(),theMeter.getStatsLastNodeCount());
    printf("STA Conns: %d\tStaDisco: %d LastDate %s",theMeter.getStatsStaConns(),theMeter.getStatsStaDiscos(),ctime(&ahora));

    printf("\n\t\t\t Location Stuff\n");
    printf("\t\t\t ============\n");
    printf("Provincia:%d Canton:%d Parroquia:%d CodigoPostal:%d\n",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal);
    printf("Expected Nodes %lu Expected Conns %lu\n",theConf.totalnodes,theConf.conns);


    printf("\n\t\t\t\t\tMeterData\n");
    printf("\t\t\t\t\t=========\n");
    printf("MeterId [%s]\tBPK[%d]\tkWh[%d]\tBeats[%d]\tLock[%s]\tPayMode[%s]\tMaxAmp[%d]\tMinAmp[%d]\n\n", theMeter.getMID(),theMeter.getBPK(),theMeter.getLkwh(),  theMeter.getBeats(),
    theMeter.getOnOff()==0?"Disc":"Conn",theMeter.getPay()==0?"POST":"PREPAID",theMeter.getMaxamp(),theMeter.getMinamp());
    vTaskDelete(NULL);
}

int cmdConfig(int argc, char **argv)
{

    xTaskCreate(&showconf,"show",4096,NULL, 10, NULL); 	        // long display allow for others to do stuff
    return 0;
}

