// #include "includes.h"
#ifndef _meterC_H_
#define _meterC_H_
#include "includes.h"


typedef struct statsm{
    uint32_t    msgIn,msgOut,bytesIn,bytesOut;
    uint8_t     lastNodeCount,lastMeterCount,staCons,staDisco;
    time_t      lastCountTS;
}stats_t;
typedef struct elmeter{
        uint16_t    beat;
        char        mid[12];
        uint8_t     paymode,onoff;          //16
        int32_t     prebal;  //can be negative  //20
        uint16_t    maxamp,minamp; //24
        uint32_t    bpk,beatlife,kwhstart;
        time_t      lastupdate;
        uint32_t    lifekwh;
        time_t      lifedate,lastclock; //52
        uint16_t    months[12];                        // 76
        uint8_t     monthHours[12][24];     //
        uint32_t    framWrites;
        uint32_t    framReads;
        uint32_t    pulseerrs;
        uint32_t    centinel;
        time_t      lastDate;
        stats_t     theStats;
} meter_def_t;

class meterClass final {
public:
    meterClass();  //constructor-> must receive the pin # to setup the pcnt

    void        loadMeter();               // load fram to internal attributes
    void        saveMeter();               // write attributes to fram
    // setters of some attributes

    void        setBeats(uint16_t hmany);
    void        setBPK(uint32_t hmany);
    void        setKstart(uint32_t hmany);
    void        setLkwh(uint32_t hmany);
    void        setMaxamp(uint16_t hmany);
    void        setMinamp(uint16_t hmany);
    void        setPrebal(uint32_t hmany);
    void        setPay(uint8_t hmany);
    void        setOnOff(uint8_t hmany);
    void        setMID(char * mid);
    void        setLifedate(time_t when);
    void        setReservedDate(time_t theDate);

//getters
    uint16_t    getBeats();
    uint32_t    getBeatsLife();
    uint16_t    getBPK();
    uint32_t    getKstart();
    uint32_t    getLkwh();
    uint16_t    getMaxamp();
    uint16_t    getMinamp();
    uint32_t    getPrebal();
    uint8_t     getPay();
    uint8_t     getOnOff();
    char *      getMID();
    uint16_t    getMonth(uint8_t mes);
    uint8_t     getMonthHour(uint8_t mes, uint8_t hora);
    time_t      getLastUpdate();
    time_t      getLifedate();
    time_t      getReservedDate();


// logic routines
    int         initMeter(uint8_t pin);
    void        addBeat();         //will have the logic of Main_app, couting beats and controlling fram
    void        saveConfig();
    void        deinit();
    void        format();             
    void        eraseMeter();
    void        turn_onoff_meter(bool turnoff,bool wrt);
    void        writeCreationDate(time_t ddate);
    time_t      readCreationDate();
    void        pcnt_init(uint16_t bbpk,uint8_t elPin);
    void        fram_Writes();
    uint32_t    getFram_Writes();
    void        fram_Reads();
    uint32_t    getFram_Reads();
    void        ISR_Lost_Pulses();
    uint32_t    getLost_Pulses();
    //stats stuff
    //getters
    uint32_t    getStatsMsgIn();
    uint32_t    getStatsMsgOut();
    uint32_t    getStatsBytesIn();
    uint32_t    getStatsBytesOut();
    uint8_t     getStatsLastNodeCount();
    uint8_t     getStatsLastMeterCount();
    uint8_t     getStatsStaConns();
    uint8_t     getStatsStaDiscos();
    time_t      getStatsLastCountTS();
    //setters
    void        setStatsLastNodeCount(uint8_t hmany);
    void        setStatsLastMeterCount(uint8_t hmany);
    void        setStatsLastCountTS(time_t now);
    void        setStatsStaConns();
    void        setStatsStaDiscos();
    void        setStatsMsgIn();    
    void        setStatsMsgOut();    
    void        setStatsBytesIn(uint32_t hmany);    
    void        setStatsBytesOut(uint32_t hmany);    

TaskHandle_t    getMainTask();    


    // bool IRAM_ATTR meter_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx);

private:
        meter_def_t                 framConfig;
        FramI2C                     fram;
        SemaphoreHandle_t           framSem;
        bool                        framFlag;
        float                       AMPSHORA;
        TaskHandle_t                main_task_handle;
        // uint32_t                    framWrites, framReads,lost_pulses;


};
#endif