#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
extern void start_ota();


int cmdOTA(int argc, char **argv)
{

   start_ota(); 	        // long display allow for others to do stuff
    return 0;
}

