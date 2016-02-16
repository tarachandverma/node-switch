#ifndef __MODAUTH_PRODUCT_MAPPINGS_CORE__H_
#define __MODAUTH_PRODUCT_MAPPINGS_CORE__H_
#include <apr_general.h>
#include <apr_pools.h>
#include <apr_tables.h>
#include <apache_typedefs.h>
#include "config_bindings2.h"
#include <shm_datatypes.h>
	char* switchcore_initialize(pool* p,shared_heap* sheap,cc_globals* globals,cc_service_descriptor* svcdesc,void** userdata);
	char* switchcore_refresh(pool* p,shared_heap* sheap,cc_globals* globals,cc_service_descriptor* svcdesc,void** userdata);
	char* switchcore_postRefresh(pool* p,shared_heap* sheap,cc_globals* globals,cc_service_descriptor* svcdesc,void** userdata);
#endif
