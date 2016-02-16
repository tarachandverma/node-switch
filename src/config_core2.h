#ifndef __WSJACL_CONFIG_CORE__H_
#define __WSJACL_CONFIG_CORE__H_

#include <sys/types.h>
#include <apache_typedefs.h>
#include <shm_datatypes.h>
#include <shm_apr.h>
#include "config_bindings2.h"
#include "config_messaging2.h"
#include "switch_config_core.h"

	typedef char* (*serviceInit) (pool*,shared_heap*,cc_globals*,cc_service_descriptor*,void**);
	typedef char* (*serviceRefresh)(pool* p, shared_heap*,cc_globals*,cc_service_descriptor*,void**);
	typedef char* (*servicePostRefresh) (pool*,shared_heap*,cc_globals*,cc_service_descriptor*,void**);
	typedef struct service_module_template{
		char* id;
		char* description;
		serviceInit init_func;
		serviceRefresh refresh_func;
		servicePostRefresh postInit_func;
	}service_module_template;
	
	typedef struct config_core{
		cc_globals* globals;
		apr_array_header_t* services;
		shared_heap* sheap;
		char* sheapMapFile;
		int sheapPageSize;
		apr_hash_t* service_id_configs;
		apr_hash_t* service_name_configs;
		cm_broker* messageBroker;
		char* databaseFile;
		char* refreshLogFile;
		int disableProcessRecovery;
		int enableAutoRefresh;
		int autoRefreshWaitSeconds;
		int threaded;
	}config_core;
	
	config_core* cc_newConfigCoreObj(pool* p);
	char* cc_loadConfigCoreFile(pool* p, char* file, config_core* conf);
	char* cc_initializeConfigCore(pool* p,config_core* conf);
	void cc_printConfigCoreDetails(pool* p,config_core* conf);
	void* cc_getModuleConfigByName(config_core* conf,char* name);
	void* cc_getModuleConfigById(config_core* conf,char* id);
//	void cc_dispModuleConfigHtml(request_rec* r,config_core* conf,char* id);
	service_module_template* cc_findServiceTemplate(char* id);
	int cc_syncSelf(apr_pool_t* pool,config_core* configCore);
	
	static const service_module_template cc_service_module_templates[]={
		{"SWITCH-CONFIG","Runtime switches configuration",switchcore_initialize,switchcore_refresh,switchcore_postRefresh}
	};
#endif

