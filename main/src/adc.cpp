#define GLOBAL
#include "globals.h"
#include "includes.h"

#define ADC_UNIT                    ADC_UNIT_1
#define _ADC_UNIT_STR(unit)         #unit
#define ADC_UNIT_STR(unit)          _ADC_UNIT_STR(unit)
#define ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define ADC_ATTEN                   ADC_ATTEN_DB_0
#define ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define ADC_GET_DATA(p_data)        ((p_data)->type1.data)
#else
#define ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define ADC_GET_DATA(p_data)        ((p_data)->type2.data)
#endif


const static int ADCSTART_BIT 			= BIT8;
const static int ADCSTOP_BIT 			= BIT9;

static adc_channel_t channel[1] = {ADC_CHANNEL_6};      //pin 34 last one in pcb

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(adc_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = ADC_BUFF,
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = theConf.gFreq*1000,
        // .sample_freq_hz = 20 * 1000,
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = ADC_UNIT;
        adc_pattern[i].bit_width = ADC_BIT_WIDTH;

        printf( "adc_pattern[%d].atten is :%d\n", i, adc_pattern[i].atten);
        printf ("adc_pattern[%d].channel is :%d\n", i, adc_pattern[i].channel);
        printf( "adc_pattern[%d].unit is :%d\n", i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK_WITHOUT_ABORT(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

void adc_task(void *pArg)
{
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[ADC_BUFF] = {0};
    memset(result, 0xcc, ADC_BUFF);
    uint32_t ivref=(uint32_t)pArg;
    int maxData=0;
    int minData=4096;
    uint16_t van=0;
    double prmAmps,maxAmps,minAmps;
    EventBits_t uxBits;
    uint32_t adcval;

    adc_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t adchandle = NULL;

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };

    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &adchandle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(adc_continuous_register_event_callbacks(adchandle, &cbs, NULL));  
    
    char unit[] = ADC_UNIT_STR(ADC_UNIT);
    prmAmps=0.0;
    
    printf("ADC setup done start adc loop\n");

    while(1)
    {
        xEventGroupWaitBits(wifi_event_group,ADCSTART_BIT,pdTRUE,pdFALSE,portMAX_DELAY);    //wait forever, this is the starting gun, flag will be cleared
        ESP_ERROR_CHECK(adc_continuous_start(adchandle));
        while (true) // adc loop
        {
            van=0;
            maxAmps=0.0;
            minAmps=500.0;
            maxData=0;
            minData=4096;   
            prmAmps=0.0; 
            while(van<ADCLOOP)     //groups of X requests to ADC
            {
            //wait for callback
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                van++;
                ret = adc_continuous_read(adchandle, result, ADC_BUFF, &ret_num, 0);
                if (ret == ESP_OK) {
                    adcval=0;
                    //  printf("ret is %x, ret_num is %d bytes\n", ret, ret_num);
                    for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                        adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i];
                        uint32_t chan_num = ADC_GET_CHANNEL(p);
                        uint32_t data = ADC_GET_DATA(p);
                        /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                        if (chan_num < SOC_ADC_CHANNEL_NUM(ADC_UNIT)) {
                            // printf("Unit: %s, Channel: %"PRIu32", Raw Value: %d \n", unit, chan_num, data);
                            adcval+=data;
                            if(data>maxData)
                                maxData=data;
                            if(data<minData)
                                minData=data;
                        } else 
                        printf("Invalid data [%s_%"PRIu32"_%"PRIx32"]\n", unit, chan_num, data);
                    }

                    // float result=((maxData-minData-theConf.biasAmp)*1.6)/4096.0;
                    float result=((maxData-minData-theConf.biasAmp)*theConf.gVref)/4096.0;
                    float VRMS= result/1.0*0.707;
                    float AmpsRMS = (VRMS * 1000)/66;
                    prmAmps=prmAmps+AmpsRMS;
                    printf("Min %d Max %d result %.04f Amps %.2f\n",minData,maxData,result,AmpsRMS);
                    if (AmpsRMS>maxAmps)
                        maxAmps=AmpsRMS;
                    if(AmpsRMS<minAmps)
                        minAmps=AmpsRMS;
                } else if (ret == ESP_ERR_TIMEOUT) {
                    //We try to read `ADC_BUFF` until API returns timeout, which means there's no available data
                    break;
                }
            }
            printf("Avg Amps %0.3f in %d passes MaxAmp %.02f MinAmp %.2f\n",prmAmps/van,van, maxAmps,minAmps);
            // check min and maxs
            if(maxAmps<theConf.minAmp)
                printf("MinAMps reached\n");
            if(maxAmps>theConf.maxAmp)
                printf("MaxAMps reached\n");
            uxBits=xEventGroupWaitBits(wifi_event_group,ADCSTOP_BIT,pdTRUE,pdFALSE,pdMS_TO_TICKS(100));    //wait a little like a delay
            if( ( uxBits & ADCSTOP_BIT ) != 0 )
            {
                // stop fired
                goto lejos;
            }

         }
   lejos:
    // printf("Adc stopped\n");
    ESP_ERROR_CHECK_WITHOUT_ABORT(adc_continuous_stop(adchandle));
//     // ESP_ERROR_CHECK_WITHOUT_ABORT(adc_continuous_deinit(adchandle));
    }
    // should never get here but...
    printf("Warning. ADC Broken Arrow\n");
    vTaskDelete(NULL);
}
