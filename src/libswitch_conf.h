#ifndef __LIBSWITCH_CONF__H_
#define __LIBSWITCH_CONF__H_

#include <sys/types.h>
#include <apache_typedefs.h>
#include <shm_data.h>
#include "config_core2.h"
#include "switch_config.h"
#include "libswitch_version.h"

#define REQUIRED_PARAMS			4

#define LS_SUCCESS				0
#define LS_FAILURE				1

typedef int ls_status_t;

typedef struct{
	int loaded_params;
	time_t uptime;
}boot_status;

typedef struct {
  boot_status boot_info;
  //shm_details shmdetails;
  char* systemInfo;
  char* home_dir;
  char* config_core_file;
  char* logfile_name;
  config_core* configCore;
  switch_config* switchConfig;
} Config;

#define	ACLCONF_OUT(config,msg){/*if(config->debug!=0){printf("%s\n",msg);}*/}
#define	ACLCONF_OUT1(config,tem,msg){/*if(config->debug!=0){printf(tem,msg);}*/}
#define	ACLCONF_OUT2(config,tem,msg1,msg2){/*if(config->debug!=0){printf(tem,msg1,msg2);}*/}

int libswitch_validBootStatus(boot_status* bstat);

//config setup functions
char* libswitch_setSystemInfo(pool* p, Config* config, char* arg);
char* libswitch_setHomeDir(pool* p, Config* config,char* arg);
char* libswitch_setConfigCoreFile(pool* p, Config* config,char* arg);
char* libswitch_setLogFile(pool* p, Config* config,char* arg);

//char* libswitch_initSheapMain(pool* p, Config* config);
int libswitch_postConfig(apr_pool_t* p, apr_pool_t* plog, apr_pool_t* ptemp, Config* config);
char* libswitch_postRefreshBind(apr_pool_t* p,Config* config);

ls_status_t libswitch_initializeLibSwitch(const char* configFile);

int libswitch_isSwitchEnabled(const char* moduleId, const char* switchName, const char* switchValue);

#endif //#if __LIBSWITCH_CONF__H_
