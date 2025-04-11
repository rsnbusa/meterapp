#ifndef TYPES_H_
#define TYPES_H_
#include "includes.h"

typedef enum webStates{wNONE,wLOGIN,wMENU,wSETUP,wCHALL} wstate_t;

//kbd definitions 
typedef struct fram{
    struct arg_str *format;         //format WHOLE fram
    struct arg_str *midw;            // read working meter id
    struct arg_int *midr;            // write working meter id
    struct arg_int *kwhstart;       // write working meter kwh start
    struct arg_int *rstart;         // read working meter kwh start
    struct arg_int *mtime;          // write working meter datetime
    struct arg_int *mbeat;          // write working meter beatstart aka beat
    struct arg_str *stats;          // init meter internal values
    struct arg_end *end;
} framArgs_t;

typedef struct logl{
    struct arg_str *level;        
    struct arg_end *end;
} loglevel_t;

typedef struct timerl{
    
    struct arg_int *first; 
    struct arg_int *repeat;          // init meter internal values
    struct arg_end *end;
} timerl_t;

typedef struct prepaid{
    
    struct arg_int *unit; 
    struct arg_int *balance;          // init meter internal values
    struct arg_end *end;
} prepaid_t;

typedef struct aes_en_ec{
    struct arg_int *key;        
    struct arg_end *end;
} aes_en_dec_t;

typedef struct resetconf{
    struct arg_str *cflags;        
    struct arg_end *end;
} resetconf_t;

typedef struct securit{
    struct arg_str *password; 
    struct arg_str *newpass; 
    struct arg_str *nopass; 
    struct arg_end *end;
} securit_t;

typedef struct logop{
    struct arg_int *show;
    struct arg_int *erase;
    struct  arg_end *end;
} logargs_t;

typedef struct adct{
    
    struct arg_int *start; 
    struct arg_int *kill;     
    struct arg_dbl *vref;   
    struct arg_int  *freq;  
    struct arg_int  *bias;  
    struct arg_int  *delay;  
    struct arg_dbl  *minAmp;  
    struct arg_dbl  *maxAmp;  
    struct arg_end *end;
} adc_t;

//end kbd defs
typedef struct medidores_mac{
    mesh_addr_t     big_table[MAXNODES];
    bool            received[MAXNODES];
    void            *thedata[MAXNODES];
    char            meterName[MAXNODES][20];
    uint32_t        lastkwh[MAXNODES];
    uint8_t         skipcounter[MAXNODES];
    bool            sendit[MAXNODES];
    uint8_t         onoff[MAXNODES];
} medidores_mac_t;

typedef struct master_Node{
    medidores_mac_t     theTable;
    int                 existing_nodes;
}master_node_t;

typedef struct mqttMsgInt{
	uint8_t 	*message;	// memory of message. MUST be freed by the Submode Routine and allocated by caller
	uint16_t	msgLen;
	char		*queueName;	// queue name to send
	uint32_t	maxTime;	//max ms to wait
}mqttMsg_t;


typedef int (*functrsn)(void *);

typedef struct cmdRecord{
    char 		comando[20];
    char        abr[6];
    functrsn 	code;
    uint32_t	count;
}cmdRecord;

typedef struct mqttRecord{
    char        * msg,*queue;
    uint16_t    lenMsg;
    functrsn 	code;
    uint32_t    *param;
}mqttSender_t;

typedef struct medbkcup{
    char        mid[13];
    uint16_t    bpk;
    time_t      backdate;
    uint32_t    kwhstart;
} med_bck;

typedef struct config {
    time_t 		bornDate;
    uint32_t 	bootcount,lastResetCode,centinel;
    uint8_t		provincia,canton,parroquia;
    uint32_t	codpostal,controllerid;
    uint32_t    downtime;               //downtime accumulator
    uint32_t    mqttSlots;          //slot number
    uint16_t    loglevel;
    uint8_t     meshconf,meterconf,ptch;
    uint32_t    lastRebootTime,meterconfdate,baset,cid,subnode,meshid;
    char        mqttServer[100];
    char        mqttUser[50];
    char        mqttPass[50];
    char        thessid[40], thepass[20];
    uint32_t    totalnodes;
    uint16_t    conns;
    uint16_t    repeat;
    char        kpass[20];
    time_t      lastKnownDate;
    char        instMsg[300];
    double      biasAmp,gVref,minAmp,maxAmp;
    int         gFreq,adcDelay;
    int         mqttcertlen;
    char        mqttcert[2000];
    uint16_t    instMsglen;
    med_bck     medback;
    uint8_t     useSec;
    char        OTAURL[100];
    uint8_t     maxSkips;
    bool        allowSkips;
} config_flash;

typedef struct meshp{
    mesh_addr_t *from;
    mesh_data_t *data;
} meshp_t;
typedef struct meterType2{          //exact match to fram struct
    uint16_t beat;              //2
    char        mid[12];               // 14
    uint8_t     paymode,onoff;          //16
    int32_t     prebal;  //can be negative  //20
    uint16_t    maxamp,minamp; //24
    uint32_t    bpk,beatlife,kwhstart,lastupdate,lifekwh,lifedate,lastclock; //52
    uint16_t    months[12];                        // 76
    uint8_t     monthHours[12][24];                 // 288+76 =364
    uint32_t    framWrites;
    uint32_t    framReads;
    uint32_t    pulseerrs;
} meterType;

#pragma pack(push, 2)       //algin on word boundary this struct and enxt one then normal c++ compiler reqiuirements
typedef struct mesh_md_sg {
char        meter_id[12];           // 0 meter id
uint8_t     paym;                   //12 payment method
uint8_t     poweroff;               //13 locked or not
uint16_t    preval;                 //14 prepaid balance
uint32_t    kwhlife;                //16 kwh for life
uint32_t    beatlife;               //20 betas for life
uint16_t    maxamp;                 //24 max amp registered
uint16_t    monthnum;               //26 month number
uint16_t    monthkwh;               //28 monthkwh
uint32_t    intid;                  //30 units mac
uint32_t    fwr;                    //34 fram writes
uint8_t     horas[24];              //38 24 hours kwh
} mesh_msg_t;           //message size is 62


typedef struct thenode {
uint32_t    nodeid,subnode;
uint32_t    tstamp;
mesh_msg_t  metersData;
} thenode_t;                //total size 70

typedef struct thebigunion{
    // char    cmd[12];                    //cmd is forced
        union  {
            thenode_t nodedata;          // for binary data format
            char    parajson[70];      // for json which should have the same cmd
        };
    } meshunion_t;
#pragma pack(pop)

typedef struct locker{
    char * data;
    int     len;
}locker_t;

#endif
