
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"


int cmdFram(int argc, char **argv)
{

  time_t now;
  struct tm timeinfo;
  char  strftime_buf[100];

  uint32_t aca;

    int nerrors = arg_parse(argc, argv, (void **)&framArg);
    if (nerrors != 0) {
        arg_print_errors(stderr, framArg.end, argv[0]);
        return 0;
    }

 if (framArg.format->count) {
  int cuanto=0;
    theMeter.deinit();
    theMeter.format();    
    time(&now);
   //  theMeter.writeCreationDate(now);

    printf("Fram Formatted\n");
    return 0;
 }

 if (framArg.stats->count) {
    const char *que=framArg.stats->sval[0];

    if(strcmp(que,"read")==0)
    {
      time_t ahora=theMeter.getStatsLastCountTS();
      printf("====== Node stats ======\n");
      printf("Msgs In: %d Bytes In: %d\n",theMeter.getStatsBytesIn());
      printf("Msgs Out: %d Bytes Out: %d\n",theMeter.getStatsMsgOut(),theMeter.getStatsBytesOut());
      printf("Nodes %d Meters %d lastDazte %s",theMeter.getStatsLastNodeCount(),theMeter.getStatsLastMeterCount(),ctime(&ahora));
    }
 }

 if (framArg.midw->count) {
    theMeter.setMID((char*)framArg.midw->sval[0]);
    printf("MeterId set to %s\n",framArg.midw->sval[0]);
 }

 if (framArg.midr->count) {
    printf("Read MeterId [%s]\n",theMeter.getMID());
 }

 if (framArg.kwhstart->count) {
    int cuanto=framArg.kwhstart->ival[0];
    theMeter.setKstart(cuanto);
    theMeter.setLkwh(cuanto);
    printf("Kwhstart set to %d\n",cuanto);
 }

 if (framArg.rstart->count) {
    printf("Reading kwhstart %d Lifekwh %d\n",theMeter.getKstart(),theMeter.getLkwh());
 }


 if (framArg.mtime->count) {
    int cuanto=framArg.mtime->ival[0];
    time_t now;

    if(cuanto)
    {
      time(&now);
      theMeter.setLifedate(now);
      printf("Lifedate set %s",ctime(&now));

    }
    else{
      now=theMeter.getLifedate();
      printf("Lifedate read %d\n",ctime(&now));
    }
 }

 if (framArg.mbeat->count) {
    int cuanto=framArg.mbeat->ival[0];

    if(cuanto>=0)
    {
      theMeter.setBeats(cuanto);
        printf("Beat set %d\n",cuanto);

    }
    else{
      printf("Beat read %d\n",theMeter.getBeats());

    }
 }

  return 0;
}
