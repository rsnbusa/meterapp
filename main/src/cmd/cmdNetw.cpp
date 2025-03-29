#define GLOBAL
#include "globals.h"

extern void write_to_flash();
extern void writeLog(char* que);

int cmdNetw(void *argument)
{
    ESP_LOGI(MESH_TAG,"Netw Cmd");
    
    cJSON *elcmd=(cJSON *)argument;
    if(elcmd)
    {
 
/*
{"cmd:"newt","f:"123456","ssid":"myssid","ssidpassw":"qweret","reboot":"Y/N","cha":"xxxxxxxx"}
*/

        cJSON *mssid= 	    cJSON_GetObjectItem(elcmd,"ssid");
        cJSON *ssidp= 	    cJSON_GetObjectItem(elcmd,"ssidpassw");
        cJSON *reboot= 	    cJSON_GetObjectItem(elcmd,"reboot");

        writeLog((char*)"Cmd Netw processed");
        if(mssid)
        {
            strcpy(theConf.thessid,mssid->valuestring);
            strcpy(theConf.thepass,ssidp->valuestring);
            write_to_flash();
            char *aca=(char*)malloc(100);
            sprintf(aca,"Network change SSID %s",mssid->valuestring);
            writeLog(aca);
            free(aca);
        }
        if(reboot)
        {
            if (strcmp(reboot->valuestring,"Y")==0)
            {
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                esp_restart();
            }
        }

        return ESP_OK;
    }
else
    return ESP_FAIL;
}
