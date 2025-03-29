#define GLOBAL
#include "includes.h" 
#include "globals.h"
extern void writeLog(char* que);
extern void write_to_flash();


int cmdSetOTA(void *argument)
{

     cJSON *cmd=(cJSON *)argument;
     ESP_LOGI(MESH_TAG,"OTA CMD");
/*
{"cmd":"setota","f":"xxxxxx","url:"xxxx","cha":"xxxx"}
*/
    if(cmd)
    {
        cJSON *url= 		cJSON_GetObjectItem(cmd,"url");
        if(url)
        {
            if(strlen(url->valuestring)>0)
            {
                strcpy(theConf.OTAURL,url->valuestring);        //if same who cares
                write_to_flash();
                char *aca =(char*)malloc(100);
                if(aca)
                {
                    sprintf(aca,"New URL set %s",url->valuestring);
                    writeLog(aca);
                    free(aca);
                }

            }
        }
        return ESP_OK;
    }
    else
        return ESP_FAIL;
}
