#ifndef METERC_H_
#define METERC_H_
#include "meterClass.h"
#include "includes.h"
#include "typedef.h"
#include "defines.h"

extern bool                         medidorlock;
extern TimerHandle_t                beatTimer;
extern config_flash                 theConf;
extern meterClass                   theMeter;
extern uint32_t                     meterSize;
extern uint8_t                      ssignal;
// extern void main_app(void* parg);

// #define GLOBAL

extern uint32_t gCRC;

static void pulse_counter(void *pArg)   //our purpose in life is this routine
{
    uint32_t            ahora=0,dif=0,eran;
    time_t              now;
    struct tm           timeinfo;
    portBASE_TYPE       res;
    int                 unit;

    while (true) { 

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);            //wait for event, beat

        //beat received, stop watchdog and restart it
        if( xTimerIsTimerActive( beatTimer ) != pdFALSE )
                xTimerStop(beatTimer,0);
        if( xTimerStart(beatTimer, 0 ) != pdPASS )
            ESP_LOGE(MESH_TAG,"Beat Timer failed");

        theMeter.addBeat();         //add a beat logic inside the meterclass
    } 
}


static void IRAM_ATTR pulse_isr(void* arg)
{
    static TaskHandle_t theH=theMeter.getMainTask();
    BaseType_t mustYield = pdFALSE;
    static TickType_t last= xTaskGetTickCountFromISR();   
    TickType_t rnow;
    uint32_t ms;

    if(theH)
    {   
        if(!gpio_get_level((gpio_num_t)ssignal))
        {
            rnow=xTaskGetTickCountFromISR();  
            ms=pdTICKS_TO_MS(rnow-last);
            if (ms>MINTICKSISR)
            {
                // esp_rom_printf("MS %d\n",pdTICKS_TO_MS(rnow-last));
                vTaskNotifyGiveFromISR(theH, &mustYield);
            }
            else
            {
                // esp_rom_printf("t%d ",ms);
                theMeter.ISR_Lost_Pulses();
            }
            last=rnow;
        }
    }
    // else
    //     esp_rom_printf("f ");

}

void meterClass::pcnt_init(uint16_t bbpk,uint8_t elPin)
{
    gpio_config_t 	            io_conf;
    bzero(&io_conf,sizeof(io_conf));

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.pull_up_en =GPIO_PULLUP_DISABLE;     //has external pullup but still do it
	io_conf.pull_down_en =GPIO_PULLDOWN_DISABLE;
	io_conf.pin_bit_mask = (1ULL<<elPin);     //input pins
	gpio_config(&io_conf);

    gpio_install_isr_service(0);
   //  MAIN APP must be launch BEFORE starting the ISR PCNT so as to receive the incoming pulses

if(theConf.meterconf>0)     //0 is not configured and hence will crash 1 is pending confirmation and 2 is confirmed, either works for the counter
{
    if(xTaskCreate(&pulse_counter,"pcnt",6*1024,NULL,  (7 ), &main_task_handle)!=pdPASS)    //careful stack size 6K is ok
        ESP_LOGE(MESH_TAG,"Could not create Pulse Counter task"); 	        // this is the objective of the app
    else
    {
        ESP_LOGI(MESH_TAG,"pulse counter installed %p",main_task_handle);
    }
        gpio_isr_handler_add((gpio_num_t)elPin, pulse_isr, (void*) elPin);

}
else
    ESP_LOGW(MESH_TAG,"PCNT not started meterconf %d",theConf.meterconf);

}

meterClass:: meterClass() 
{
     bzero(&framConfig,sizeof(framConfig));
     meterSize=sizeof(framConfig);
     AMPSHORA=1000.0/VOLTS;
}

//setters
void meterClass::setBeats(uint16_t hmany)
{

    framConfig.beat+=hmany;
    saveMeter();

}

void meterClass::setBPK(uint32_t hmany)
{
    framConfig.bpk=hmany;
    saveMeter();
}

void meterClass::setKstart(uint32_t hmany)
{
    framConfig.kwhstart=hmany;
    saveMeter();
}

void meterClass::setLkwh(uint32_t hmany)
{
    framConfig.lifekwh=hmany;
    saveMeter();
}

void meterClass::setMaxamp(uint16_t hmany)
{
    framConfig.maxamp=hmany;
    saveMeter();
}

void meterClass::setMinamp(uint16_t hmany)
{
    framConfig.minamp=hmany;
    saveMeter();
}

void meterClass::setPrebal(uint32_t bal)
{
    framConfig.prebal=bal;
    turn_onoff_meter(TURNON,false);
    saveMeter();

}

void meterClass::setPay(uint8_t hmany)
{
    framConfig.paymode=hmany;
    saveMeter(); 
}

void meterClass::setOnOff(uint8_t hmany)
{
    framConfig.onoff=hmany;
    saveMeter();
}

void meterClass::setMID(char * mmid)
{
    strcpy(framConfig.mid,mmid);
    saveMeter();
}

void meterClass::setLifedate(time_t when)
{
    framConfig.lifedate=when;
    saveMeter();
}


void meterClass::setReservedDate(time_t theDate)
{
    framConfig.lastDate=theDate;
}

// getters

uint16_t meterClass::getBeats()
{
    return framConfig.beat;
}

uint32_t meterClass::getBeatsLife()
{
    return framConfig.beatlife;
}

uint16_t meterClass::getBPK()
{
    return framConfig.bpk;
}

uint32_t meterClass::getKstart()
{
    return framConfig.kwhstart;
}

uint32_t meterClass::getLkwh()
{
    return framConfig.lifekwh;
}

uint16_t meterClass::getMaxamp()
{
    return framConfig.maxamp;
}

uint16_t meterClass::getMinamp()
{
    return framConfig.minamp;
}

uint32_t meterClass::getPrebal()
{
    return framConfig.prebal;
}


uint8_t meterClass::getPay()
{
    return framConfig.paymode;
}

uint8_t meterClass::getOnOff()
{
    return framConfig.onoff;
}

char * meterClass::getMID()
{
    return framConfig.mid;
}

uint16_t meterClass::getMonth(uint8_t mes)
{
    return framConfig.months[mes];
}

uint8_t meterClass::getMonthHour(uint8_t mes, uint8_t hora)
{
    return framConfig.monthHours[mes][hora];
}


time_t meterClass::getLastUpdate()
{
    return framConfig.lastupdate;
}

time_t meterClass::getLifedate()
{
    return framConfig.lifedate;
}

time_t meterClass::getReservedDate() {

    return framConfig.lastDate;
}


void meterClass::deinit()
{
    uint32_t copyFramW=framConfig.framWrites;
    bzero(&framConfig,sizeof(framConfig));
    framConfig.framWrites=copyFramW;
}

void meterClass::format()
{   
    // uint32_t copy_fram_writes=framConfig.framWrites; //never loose count of writes events
    if(xSemaphoreTake(framSem, portMAX_DELAY/  portTICK_PERIOD_MS))		
    {
        // printf("MClass call format\n");
        fram.format(NULL,3000,true);
        xSemaphoreGive(framSem); 
    }
    // framConfig.framWrites=copy_fram_writes;

    saveMeter();

}

void meterClass::saveConfig()
{
    framConfig.lastupdate=0;
    framConfig.lifedate=0;
    framConfig.lastclock=0;
    framConfig.minamp=9999;
    framConfig.maxamp=0;   
    framConfig.prebal=framConfig.paymode=framConfig.onoff=framConfig.beat=framConfig.beatlife=0;
    bzero(&framConfig.months,sizeof(framConfig.months));
    bzero(&framConfig.monthHours,sizeof(framConfig.monthHours));
    framConfig.centinel=CENTINEL;
    //all internal now initialized, other set by configurator    
    saveMeter();
}

int meterClass::initMeter(uint8_t pin)
{

	framFlag=fram.begin(FSDA,FSCL,&framSem); //will create I2C channel and Semaphore
	if(framFlag)
	{
        loadMeter();
        if(framConfig.centinel != CENTINEL)
        {
            ESP_LOGE(MESH_TAG,"FRAM Centinel %x failed... should format FRAM",framConfig.centinel);
            // delay(3000);
            // fcenti=0x12345678;
            // fram.writeMany(FCENTINEL,(uint8_t*)&fcenti,4);
            // fram.format(0,NULL,1000,true);
            // send_911_call("No Fram FATAL","FramInit");
            return ESP_FAIL;
        }
//stats are now part of th emeterfram data itself

        if(framConfig.lifekwh>framConfig.kwhstart)  // see if meter has had activity 
            medidorlock=true;    //lock it, cannot be changed
        else   
            medidorlock=false; 
    }
    else
        return ESP_FAIL;
//now setup PCNT
        if(framConfig.bpk!=0)
        {
            pcnt_init(framConfig.bpk,pin);
        }

// prepaid balance check

        if(framConfig.paymode==PREPAID)
        {
            if(framConfig.prebal==0)
                {
                    //turn off the Meter
                    ESP_LOGI(MESH_TAG,"Turn Meter off, no balance %d",framConfig.prebal);
                    turn_onoff_meter(TURNOFF,true);
                    framConfig.onoff=1;
                }
        }
        // printf("Turn meter %s\n",medidor.onoff?"ON":"OFF");
        turn_onoff_meter(framConfig.onoff,true);
    return ESP_OK;
}

void meterClass::loadMeter()
{
    if(xSemaphoreTake(framSem, portMAX_DELAY/  portTICK_PERIOD_MS))		
    { 
        fram.read_meter((uint8_t*)&framConfig,sizeof(framConfig));
        xSemaphoreGive(framSem);
    }
    fram_Reads();

}

void meterClass::turn_onoff_meter(bool turnoff,bool wrt)
{
    int err;

    if(turnoff==TURNOFF)
    {                    
        framConfig.onoff=TURNOFF;    
        if(err>0)
            ESP_LOGE(MESH_TAG,"error turning off err=%x",err);

        else
            ESP_LOGI(MESH_TAG,"Meter turned off");
        gpio_set_level((gpio_num_t)RELAY,RELAYOFF);

    }
    else
    {
        framConfig.onoff=TURNON;    
        if(err>0)
        {
            ESP_LOGE(MESH_TAG,"error turning on unit err=%x",err);
        }
        else
           ESP_LOGI(MESH_TAG,"Meter turned on");
        gpio_set_level((gpio_num_t)RELAY,RELAYON);

    }

    if(wrt) 
        saveMeter();

}

void meterClass::addBeat()
{
    uint32_t            ahora=0,dif=0,eran;
    time_t              now;
    struct tm           timeinfo;
    portBASE_TYPE       res;
    int                 unit,suma;
    uint32_t            amps;
    uint16_t            dia,mes,hora;

    if (framConfig.bpk==0)
    {
        framConfig.bpk=1000;
        ESP_LOGE(MESH_TAG,"No data from FRAM...should stop");
    }

        suma=framConfig.beat % (framConfig.bpk/100);
        ahora=pdTICKS_TO_MS(xTaskGetTickCount());
        if(framConfig.lastclock>0)
            dif=(ahora-framConfig.lastclock);
        else
            dif=0;

        eran= framConfig.beat;
        framConfig.lastclock=ahora;
        framConfig.beat++;
        framConfig.beatlife++;
        if(dif>0)
        {
            // 1kw@120v is 8.33 Amps (1000w/120v) VOLTS is estimated voltage 
            // 1 hour has 3600000 ms
            // meter beats at BPK per kw so it takes 3600000/BPK ms per beat to get to 8.33 amps
            // final formula then 3600000ms/bpk  divided by time between beats * amps 1kw 
            // if beats faster, dif ms between beats is smaller than 3600000ms/bpk and  division increases amps, similar if time ms increases amps decrease

                amps=(3600000.0/(float)framConfig.bpk)/(float)dif*AMPSHORA;    
                if((int)amps>framConfig.maxamp)
                {
                    framConfig.maxamp=(uint16_t)amps;
                    ESP_LOGW(MESH_TAG,"New maxAmp %d ms %d",framConfig.maxamp,dif);
                }

                if((int)amps<framConfig.minamp && (int)amps>0)
                {
                    framConfig.minamp=(uint16_t)amps;
                    ESP_LOGW(MESH_TAG,"New minAmp %d ms %d",framConfig.minamp,dif);
                }
        }
        if( framConfig.beat>=framConfig.bpk)
        { 
            // BPK achived, update and save
            // prepaid checking here 
            time(&now);
            localtime_r(&now, &timeinfo);
            dia=timeinfo.tm_yday;
            mes=timeinfo.tm_mon;
            hora=timeinfo.tm_hour;

            // time(&now);
            framConfig.lifekwh++;
            framConfig.monthHours[mes][hora]++;
            framConfig.months[mes]++;
            framConfig.lastupdate=now;
            // beat=0;
            framConfig.beat=framConfig.beat % framConfig.bpk;;
            suma=0;

            if(framConfig.prebal>0)
            { //maybe some good faith credit that when user adds kwhs is rested from balance? % or fixed? fixed
                framConfig.prebal--;
                if(framConfig.prebal<MINPREPAID)
                {
                        ESP_LOGI(MESH_TAG,"Turn Meter %d off, no balance %d",unit,framConfig.prebal);
                        turn_onoff_meter(TURNOFF,false);                //will be saved below
                }
            }
                framConfig.lastDate=now;
                saveMeter();     
        }
        else
        {   
            suma++;
            if( (suma% (framConfig.bpk/100))==0)
            {
                    time(&now);
                    // printf("Beats %d Life %d Unit %d\n",beat,beatlife,unit);
                    if(framConfig.lifedate==0)
                        framConfig.lifedate=(int)now;     //first pulse is Birth Date
                    framConfig.lastDate=now;
                    saveMeter();
                suma=0;
            }
        }
    } 

    void meterClass::saveMeter()        //used by beat timer or any routine jneeds to save meter NOW
    {
        fram_Writes();
        if(xSemaphoreTake(framSem, portMAX_DELAY/  portTICK_PERIOD_MS))		
        { 
            fram.write_meter((uint8_t*)&framConfig,sizeof(framConfig));            
            xSemaphoreGive(framSem);
        }
    }

void meterClass::writeCreationDate(time_t ddate)
{        

saveMeter();      

}

void meterClass::eraseMeter()
{

    //erase kws/kwh and monthly metrics
    framConfig.maxamp=framConfig.minamp=framConfig.beatlife=framConfig.kwhstart=framConfig.lifekwh=framConfig.beat=0;
    for(int a=0;a<12;a++)
    {
        framConfig.months[a]=0;
        for(int b=0;b<24;b++)
        framConfig.monthHours[a][b]=0;
    }
}

time_t meterClass::readCreationDate()
{        
    return framConfig.lastDate;
}

TaskHandle_t meterClass::getMainTask()
{
    return main_task_handle;
}
 
void meterClass::fram_Writes()
{
    framConfig.framWrites++;
}

void meterClass::fram_Reads()
{
    framConfig.framReads++;
}

void meterClass::ISR_Lost_Pulses()
{
    framConfig.pulseerrs++;
}

uint32_t meterClass::getLost_Pulses()
{
    return framConfig.pulseerrs;
}

uint32_t meterClass::getFram_Writes()
{
    return framConfig.framWrites;
}

uint32_t meterClass::getFram_Reads()
{
    return framConfig.framReads;
}

// stats stuff
//getters

uint32_t meterClass::getStatsMsgIn()
{
    return framConfig.theStats.msgIn;
}
uint32_t meterClass::getStatsMsgOut()
{
    return framConfig.theStats.msgOut;
}
uint32_t meterClass::getStatsBytesOut()
{
    return framConfig.theStats.bytesOut;
}
uint32_t meterClass::getStatsBytesIn()
{
    return framConfig.theStats.bytesIn;
}

uint8_t meterClass::getStatsLastNodeCount()
{
    return framConfig.theStats.lastNodeCount;
}

uint8_t meterClass::getStatsLastMeterCount()
{
    return framConfig.theStats.lastMeterCount;
}

uint8_t meterClass::getStatsStaConns()
{
    return framConfig.theStats.staCons;
}

uint8_t meterClass::getStatsStaDiscos()
{
    return framConfig.theStats.staDisco;
}

time_t meterClass::getStatsLastCountTS()
{
    return framConfig.theStats.lastCountTS;
}
//setters 

void meterClass::setStatsLastNodeCount(uint8_t hmany)
{
    framConfig.theStats.lastNodeCount=hmany;
}

void meterClass::setStatsLastMeterCount(uint8_t hmany)
{
    framConfig.theStats.lastMeterCount=hmany;
}

void meterClass::setStatsLastCountTS(time_t now)
{
    framConfig.theStats.lastCountTS=now;
}

void meterClass::setStatsStaConns()
{
    framConfig.theStats.staCons++;
}

void meterClass::setStatsStaDiscos()
{
    framConfig.theStats.staDisco++;
}

void meterClass::setStatsMsgIn()
{
    framConfig.theStats.msgIn++;
}

void meterClass::setStatsMsgOut()
{
    framConfig.theStats.msgOut++;
}

void meterClass::setStatsBytesOut(uint32_t hmany)
{
    framConfig.theStats.bytesOut+=hmany;
}

void meterClass::setStatsBytesIn(uint32_t hmany)
{
    framConfig.theStats.bytesIn+=hmany;
}
    #endif