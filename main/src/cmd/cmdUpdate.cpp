#define GLOBAL
#include "globals.h"
extern void writeLog(char* que);
  
int cmdUpdate(void *argument)
{

     cJSON *cmd=(cJSON *)argument;
     ESP_LOGI(MESH_TAG,"Update CMD");
/*
{"cmd":"update","f":"xxxxxx","mid";"thismeter","bpk":12234,"ks":1234,"k":12313,"cha":"xxxxxxxx"}
*/
    if(cmd)
    {
        cJSON *mid= 		cJSON_GetObjectItem(cmd,"mid");
        if(mid)
        {
            //only checked required for this command before sending, NOde will do all other validations

// this just prepares the message to be sent into the MESH to the appropiate Node. He will erase and format 
            char *meshMsg=cJSON_PrintUnformatted(cmd);
            if(!meshMsg)
                return ESP_FAIL;
            
            char *aca=(char*)malloc(100);
            sprintf(aca,"Update Mid %s",mid->valuestring);
            writeLog(aca);
            free(aca);
            mesh_data_t data;
            data.proto = MESH_PROTO_BIN;
            data.tos = MESH_TOS_P2P;
            data.data=(uint8_t*)meshMsg;
            data.size=strlen(meshMsg);
            int err= esp_mesh_send( &GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);         //broadcast for appropiate node
            free(meshMsg);
        }
        else
            return ESP_FAIL;
        return ESP_OK;
    }
    else
        return ESP_FAIL;
}
