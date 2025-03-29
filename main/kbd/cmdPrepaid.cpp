
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"


int cmdPrepaid(int argc, char **argv)
{

  int nerrors = arg_parse(argc, argv, (void **)&prepaidcmd);
  if (nerrors != 0) {
      arg_print_errors(stderr, prepaidcmd.end, argv[0]);
      return 0;
  }
  if (prepaidcmd.unit->count) 
  {
      int unitt=prepaidcmd.unit->ival[0];
      if(unitt<8)
      {
        if(theMeter.getPay()==PREPAID)
        {
          if (prepaidcmd.balance->count) 
          {
            int era=theMeter.getPrebal();
            int bal=prepaidcmd.balance->ival[0];
            theMeter.setPrebal(era+bal);
            // fram.write_prebal(unitt,medidor.prebal);
            // printf("Unit %d new balance of %d kwh\n",unitt,medidor.prebal);
            // if(era<0)
            // {
              // turn_onoff_meter(TURNON);  shoud be sett with theMetert
              // pcnt_unit_start(pcnt_unit[unitt]);
            // }
            // medidor.onoff=TURNON;                    

          }
          else
          {
            printf("No balance given for unit %d\n",unitt);
            return 0;
          }
        }
        else
        {
          printf("Unit %d is not Prepaid %d\n",theMeter.getPay());
          return 0;
        }
      }
      else
      {
        printf("Unit OB %d\n",unitt);
        return 0;
      }
    }
  else
  {
    printf("Prepaid no unit specified\n");
  }
  return 0;
}
