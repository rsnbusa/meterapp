#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern void adc_task(void* parg);

int cmdAdc(int argc, char **argv)
{
  int cuanto;
    
    int nerrors = arg_parse(argc, argv, (void **)&adcArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr,adcArgs.end, argv[0]);
        return 0;
    }
    if(adcArgs.start->count)
    {
        uint32_t lo=adcArgs.start->ival[0];
        printf("Start ADC %d loops Vref %.2f Freq %d Bias %d\n",lo,theConf.gVref,theConf.gFreq,theConf.biasAmp);
        xTaskCreate(&adc_task,"adc",8*1024,NULL,  (7 ), &adc_task_handle); 	        // this is the objective of the app
        return 0;
    }
    if(adcArgs.kill->count)
    {
      printf("ADC Task killed\n");
      vTaskDelete(adc_task_handle);
      return 0;
    }

    if(adcArgs.vref->count)
    {
      theConf.gVref=adcArgs.vref->dval[0];
      printf("VRef set to %.2f\n",theConf.gVref);
    }
    if(adcArgs.minAmp->count)
    {
      theConf.minAmp=adcArgs.minAmp->dval[0];
      printf("minAmp set to %.2f\n",theConf.minAmp);
    }
    if(adcArgs.maxAmp->count)
    {
      theConf.maxAmp=adcArgs.maxAmp->dval[0];
      printf("maxAmp set to %.2f\n",theConf.maxAmp);
    }

    if(adcArgs.freq->count)
    {
      cuanto=adcArgs.freq->ival[0];
      if(cuanto<20)
          printf("Invalid range >=20\n ");
      else
        {
          theConf.gFreq=cuanto;
          printf("Freq set to %d\n",theConf.gFreq);
        }
    }
    if(adcArgs.bias->count)
    {
      theConf.biasAmp=adcArgs.bias->ival[0];
      printf("Bias set to %d\n",theConf.biasAmp);
    }
    if(adcArgs.delay->count)
    {
      theConf.adcDelay=adcArgs.delay->ival[0];
      printf("Delay set to %d\n",theConf.adcDelay);
    }
    write_to_flash();
  return 0;
}

