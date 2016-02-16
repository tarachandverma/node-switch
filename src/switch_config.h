#ifndef __MODAUTH_PRODUCT_MAPPINGS__H_
#define __MODAUTH_PRODUCT_MAPPINGS__H_
#include <apr_general.h>
#include <apr_pools.h>
#include <apr_tables.h>
#include <apache_typedefs.h>
#include <shm_data.h>
#include <shm_apr.h>
#include "switch_types.h"
#include "config_bindings2.h"

	typedef struct switch_element{
		char* name;
		switch_type type;
		void* value;
	}switch_element;
	
	typedef struct switch_module{
		char* id;
		shmapr_hash_t* params;
	}switch_module;
	
	typedef struct switch_config{
		shmapr_hash_t* modules;
	}switch_config;
	
	char* switchconf_build(pool* p,shared_heap* sheap, int isRefresh,cc_globals* globals,char* filepath);
	void switchconf_printSwitchConfigDetails(pool* p,switch_config* conf);
	switch_config* switchconf_fetchFromSheap(shared_heap* sheap);
	int switchconf_matchByStrings(pool* p, char* regex, char* value);
	
#endif
