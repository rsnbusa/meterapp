#define GLOBAL
#include "includes.h" 
#include "globals.h"
extern void start_ota();

int cmdHostOTA(void *argument)
{

     cJSON *cmd=(cJSON *)argument;
     ESP_LOGI(MESH_TAG,"OTA CMD");
/*
{"cmd":"ota","f":"xxxxxx","cha":"xxxx"}
*/
    if(cmd)
    {
        start_ota();
        return ESP_OK;
    }
    else
        return ESP_FAIL;
}
