#define GLOBAL
#include "includes.h" 
#include "globals.h"

meshunion_t * sendData(bool forced)
{
    time_t now;
    struct tm timeinfo;
    int thismonth;
    meshunion_t* thisNode;
    uint8_t     thismac[6];

    esp_base_mac_addr_get((uint8_t*)thismac);
    time(&now);
    localtime_r(&now, &timeinfo);
    thismonth=timeinfo.tm_mon;
    thisNode=(meshunion_t*)malloc(sizeof(meshunion_t));
    if(!thisNode)
    {
        printf("error sendata no RAM\n");
        return NULL;
    }
    bzero(thisNode, sizeof(meshunion_t));

    thisNode->nodedata.tstamp=now;
    thisNode->nodedata.nodeid=theConf.controllerid;
    thisNode->nodedata.subnode=theConf.subnode;
//into binary format of union

        strcpy(thisNode->nodedata.metersData.meter_id,theMeter.getMID());
        thisNode->nodedata.metersData.kwhlife=theMeter.getLkwh();
        thisNode->nodedata.metersData.monthkwh=theMeter.getMonth(thismonth);
        thisNode->nodedata.metersData.beatlife=theMeter.getBeats();
        thisNode->nodedata.metersData.maxamp=theMeter.getMaxamp();
        thisNode->nodedata.metersData.paym=theMeter.getPay();
        thisNode->nodedata.metersData.poweroff=theMeter.getOnOff();
        thisNode->nodedata.metersData.preval=theMeter.getPrebal();
        thisNode->nodedata.metersData.monthnum=thismonth;
        memcpy(&thisNode->nodedata.metersData.intid,&thismac[2],4);;
        thisNode->nodedata.metersData.fwr=theMeter.getFram_Writes();        //important to know expiration of fraw which should never happen

        for (int b=0;b<24;b++)
            thisNode->nodedata.metersData.horas[b]=theMeter.getMonthHour(thismonth,b);
        
    
    return thisNode;        //must be freed by caller
}
