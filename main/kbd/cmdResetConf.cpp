#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern void erase_config();
extern void root_collect_meter_data(TimerHandle_t  algo);

char    resetf[6][10]={"ALL","METER","MESH","COLLECT","SEND","MQTT"};

int findFlag(char * cual)
{
  for (int a=0;a<6;a++)
    if(strcmp(cual,resetf[a])==0)
      return a;
  
  return -1;
}


int cmdResetConf(int argc, char **argv)
{
  uint32_t pop=0;
  mqttSender_t mensaje;
  char *mqttmsg;
  mesh_data_t data;
  wifi_config_t       configsta;
  int err,lev;
  char  aca[10];

  int nerrors = arg_parse(argc, argv, (void **)&resetlevel);
  if (nerrors != 0) {
      arg_print_errors(stderr, resetlevel.end, argv[0]);
      return 0;
  }
  if (resetlevel.cflags->count) 
  {
    strcpy(aca,resetlevel.cflags->sval[0]);
    for (int x=0; x<strlen(aca); x++)
        aca[x]=toupper(aca[x]);
    lev=findFlag(aca);
    if(lev>=0)
    {
      // printf("Flags set to %s/%d\n",aca,lev);
      switch(lev) {
        case 0:
          printf("All flags resetted\n");
          theConf.meterconf=0;
          theConf.meshconf=0;
          erase_config();
          break;
        case 1:
          printf("Meter resetted\n");
          theConf.meterconf=0;
          break;
        case 2:
          printf("Mesh resetted\n");
          theConf.meshconf=0;
          break;
        case 3:
          printf("Collect Data\n");
          if(esp_mesh_is_root()) //only non ROOT 
              root_collect_meter_data(NULL);
            break;
        case 4:
          printf("Sendmeterf resetted\n");
          sendMeterf=0;
          break;
        case 5:
          printf("Mqttf released\n");
          mqttf=true;
          break;
        default:
          printf("Wrong choice of reset\n");
      }
      write_to_flash();
    }
    return 0;
  }
  else 
    printf("No such Flag\n");
  return 0;
}

