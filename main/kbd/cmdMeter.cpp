#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"



int cmdMeter(int argc, char **argv)
{
  // char update[80],initd[0];
  struct tm *timeinfo;
  bool mmes=false, hhora=false;
  char *uupdate=(char*)malloc(100);
  char *iinitd=(char*)malloc(100);
  time_t mio;
  int pulse_count;

  for (int a=0;a<MAXDEVSS;a++)
  {      
    // printf("lastupdate %lu\n",medidor.lastupdate);
    mio=theMeter.getLastUpdate();
    timeinfo=localtime((time_t*)&mio);
    strftime(uupdate, 100, "%c", timeinfo);
    mio=theMeter.getLifedate();
    timeinfo=localtime((time_t*)&mio);
    strftime(iinitd, 100, "%c", timeinfo);
    if(strlen(theMeter.getMID())>0)
    {
  
      // ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit[a], &pulse_count));
      printf("Meter[%d] [id=%s] [BPK %d]  [Beat %d]  [Beatlife %d]  [kwhStart %d]  [KwhLife %d] PayMode [%s] Prebal [%d] OnOff [%s]",a,theMeter.getMID(),theMeter.getBPK(),
      theMeter.getBeats(),theMeter.getBeatsLife(),theMeter.getKstart(),theMeter.getLkwh(),theMeter.getPay()==0?"POST":"PREPAID",
      theMeter.getPrebal(),theMeter.getOnOff()==0?"ON":"OFF" );
      printf(" MaxAmp [%d] MinAmp [%d] [Update %s ] [Initd %s]\n",theMeter.getMaxamp(),theMeter.getMinamp(),uupdate,iinitd);
      for(int b=0;b<12;b++)
      {
        if(theMeter.getMonth(b)>0)
        {
          uint32_t check=0;
          printf("Month[%d]=%d ",b,theMeter.getMonth(b));
          mmes=true;
          for (int c=0;c<24;c++)
          {
            if(theMeter.getMonthHour(b,c)>0)
            {
              printf("Hour[%d]=%d ",c,theMeter.getMonthHour(b,c));
              hhora=true;
              check+=theMeter.getMonthHour(b,c);
            }
          }
          if (hhora)
            printf( "check %d\n",check);
            hhora=false;
        }
        if( mmes)
          printf("\n");
          mmes=false;
      }
    }
  }
  free(uupdate);
  free(iinitd);
  return 0;
}
