#define GLOBAL

#include "globals.h"
#include "includes.h"
extern void ssdString(int x, int y, char * que,bool centerf);
extern void delay(uint32_t cuanto);
extern void writeLog(char* que);


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

void ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA task");
    //Disable self-organized networking
        delay(3000);

esp_mesh_set_self_organized(0, 0);

vTaskSuspend(mqttSendHandle);
vTaskSuspend(mqttMgrHandle);

    esp_http_client_config_t config;
    bzero(&config,sizeof(config));

        // strcpy((char*)config.url , theConf.OTAURL);
        config.url=theConf.OTAURL;
        config.event_handler = _http_event_handler;
        config.keep_alive_enable = true;

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    ESP_LOGI(TAG, "Attempting to download update from [%s]", config.url);

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        const esp_app_desc_t *mip=esp_app_get_description();
        char *aca=(char*)malloc(100);
        if(mip)
            sprintf(aca,"OTA Loaded Version %s",mip->version);
        else
            sprintf(aca,"OTA Loaded Version");

        writeLog(aca); //aca no need to free we are restarting 
        delay(1000);
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
        char *aca=(char*)malloc(100);
        sprintf(aca,"OTA Failed URL %s",theConf.OTAURL);
        writeLog(aca);  
        free(aca);
        vTaskResume(mqttSendHandle);
        vTaskResume(mqttMgrHandle);

    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void start_ota()
{
    xTaskCreate(&ota_task,"ota",10240,NULL, 5, NULL);           //start the Virtual Machine connected or not to wifi

}