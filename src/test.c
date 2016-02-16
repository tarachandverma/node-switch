#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "libswitch_conf_ext.h"
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*sec)
#endif

int main(int argc, char *argv[])
{
	int switchValue;
	char* libswitchConfigFile = "./conf/libswitch.conf";
		
	libswitch_initializeLibSwitch(libswitchConfigFile);
	
	while(1){
                printf("module-1/enableUuidCheck=%d\r\n", libswitch_isSwitchEnabled("module-1", "enableUuidCheck","on"));    fflush(stdout);
                printf("module-1/uuid=%d\r\n", libswitch_isSwitchEnabled("module-1", "uuid","dj_chandt"));    fflush(stdout);
                printf("module-1/uuidArray=%d\r\n", libswitch_isSwitchEnabled("module-1", "uuidArray","e1"));    fflush(stdout);
                printf("module-1/uuidList=%d\r\n", libswitch_isSwitchEnabled("module-1", "uuidList","dj_chandt"));    fflush(stdout);
                printf("module-2/switch1=%d\r\n", libswitch_isSwitchEnabled("module-2", "switch1","e1"));    fflush(stdout);
                printf("module-2/switch1=%d\r\n", libswitch_isSwitchEnabled("module-2", "switch2",""));    fflush(stdout);
                printf("module-3/switch1=%d\r\n", libswitch_isSwitchEnabled("module-3", "switch1",""));    fflush(stdout);
                printf("module-3/switch1=%d\r\n", libswitch_isSwitchEnabled("module-3", "switch2",""));    fflush(stdout);
		printf("\r\n"); sleep(3);
	}
   return 0;
}
