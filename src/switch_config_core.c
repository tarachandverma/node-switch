#ifndef __MODAUTH_PRODUCT_MAPPINGS_CORE__C_
#define __MODAUTH_PRODUCT_MAPPINGS_CORE__C_
#include "switch_config_core.h"
#include "switch_config.h"
#include "common_utils.h"
#define PARAM_CONFIG_XML	"config-xml"
#define PARAM_IS_REFRESH	"initialized"
	char* switchcore_initialize(pool* p,shared_heap* sheap,cc_globals* globals,cc_service_descriptor* svcdesc,void** userdata){
		char* result=NULL;
		char* error=NULL;
//		char* isRefresh=NULL;
		char* configFile=NULL;
		//temp vars


		char* param_configFile=(char*)apr_hash_get(svcdesc->params,PARAM_CONFIG_XML,APR_HASH_KEY_STRING);
		if(param_configFile==NULL){
			return apr_pstrcat(p,"Missing service param:",PARAM_CONFIG_XML,NULL);
		}
		//get Latest Action Mappings Resource
		configFile=cc_getRemoteResourcePath(p,globals,param_configFile,&error);
		if(configFile==NULL){
			configFile=cc_getLocalResourcePath(p, globals,param_configFile,NULL);
		}
		
		result=switchconf_build(p,sheap,(apr_hash_get(svcdesc->params,PARAM_IS_REFRESH,APR_HASH_KEY_STRING)!=NULL),globals,configFile);
		if(result!=NULL){
			return apr_pstrcat(p,"ProductMappings: ",result,NULL);
		}
	
		apr_hash_set(svcdesc->params,PARAM_IS_REFRESH,APR_HASH_KEY_STRING,apr_pstrdup(p,"TRUE"));
		return NULL;
	}
	char* switchcore_refresh(pool* p,shared_heap* sheap,cc_globals* globals,cc_service_descriptor* svcdesc,void** userdata){
			return switchcore_initialize(p,sheap,globals,svcdesc,userdata);
	}
	
	char* switchcore_postRefresh(pool* p,shared_heap* sheap, cc_globals* globals,cc_service_descriptor* svcdesc,void** userdata){
		switch_config* mappings=NULL;
		mappings=switchconf_fetchFromSheap(sheap);
		if(mappings==NULL){
			return apr_pstrdup(p,"Product Mappings unable to be retrieved from sheap");
		}
		*userdata=mappings;
		return NULL;	
	}
	
#endif


