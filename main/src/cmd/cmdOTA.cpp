#define GLOBAL
#include "globals.h"
extern void start_ota();
extern void writeLog(char* que);

int cmdOTA(void *argument)
{
    ESP_LOGW(MESH_TAG,"OTA Remote Command");
    start_ota();
#ifdef LOGOPT
    char *tmp=(char*)calloc(100,1);
    if(tmp)
    {
        sprintf(tmp,"OTA Command executed");
        writeLog(tmp);
        free(tmp);
    }
#endif        
    return ESP_OK;
}
