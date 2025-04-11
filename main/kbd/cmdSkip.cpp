#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

int cmdSkip(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&appSkip);
    if (nerrors != 0) {
        arg_print_errors(stderr, appSkip.end, argv[0]);
        return 0;
    }
  if (appSkip.password->count) 
  {   
    if(strcmp(appSkip.password->sval[0],theConf.kpass )==0)
    {
      if (appSkip.newpass->count)
       {
          int nn=atoi(appSkip.newpass->sval[0]);
          printf("New Skip Count from [%d] to [%d]\n",theConf.maxSkips,nn);
          theConf.maxSkips=nn;
          write_to_flash();
       }
      if (appSkip.nopass->count)
       {
          bool como=strcmp("y",appSkip.nopass->sval[0]);
          printf("New Skip Active from [%s] to [%s]\n",theConf.allowSkips?"Yes":"No",como?"No":"Yes");
          theConf.allowSkips=!como;
          write_to_flash();
       }
    }
    else
      printf("Wrong password\n");
  }

  return 0;
}
