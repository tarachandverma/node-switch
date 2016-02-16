#ifndef __WSJACL_CONFIG_BINDINGS__C_
#define __WSJACL_CONFIG_BINDINGS__C_

#include <apache_mappings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include "config_bindings2.h"
#include <common_utils.h>
#include <http-utils/http_client.h>

	cc_service_descriptor* cc_newServiceDescriptorObj(pool* p){
		cc_service_descriptor* ret=(cc_service_descriptor*)apr_palloc(p,sizeof(cc_service_descriptor));
		ret->id=NULL;
		ret->name=NULL;
		ret->uri=NULL;
		ret->userColonPass=NULL;
		ret->timeoutSeconds=2;
		ret->params=apr_hash_make(p);
		return ret;
	}
	cc_globals* cc_newGlobalsObj(pool* p){
		cc_globals* ret=(cc_globals*)apr_pcalloc(p,sizeof(cc_globals));
		ret->nameSpace=NULL;
		ret->homeDir=NULL;
		ret->logsDir=NULL;
		ret->resourceService=NULL;
		return ret;
	}
	char* cc_initGlobals(pool* p,cc_globals* globals){
		if(globals->logsDir!=NULL){
			apr_dir_make_recursive(globals->logsDir,APR_OS_DEFAULT,p);
		}
		return NULL;
	}
	
	char* cc_getRemoteResourcePath(pool* p, cc_globals* globals,char* resource,char**details){
		char* ret=NULL, * bakFile=NULL,* filename=NULL;
		http_util_result* result=NULL;
		cc_service_descriptor* rs=NULL;
		char* reqUri=NULL;
		
		//file vars
		apr_file_t* file=NULL;
		apr_status_t status;
		apr_pool_t* tp=NULL;
		apr_size_t file_written;
		char* errorMsg=NULL;
		
		if(resource==NULL) return NULL;
		
		if(globals->resourceService!=NULL){
			rs=globals->resourceService;
			//setup filepool
			if(apr_pool_create(&tp,p)!=APR_SUCCESS){
				if(details!=NULL){
					*details=apr_pstrdup(p,"Failure to create subpool");
				}
				return NULL;
			}
			
			reqUri=apr_pstrcat(p,rs->uri,"/",resource,NULL);
			//try to load from remote
			result=http_request_get_verbose(p,reqUri,rs->timeoutSeconds,rs->userColonPass,NULL,&errorMsg);

			if(http_is200_OK(result)){
				//write file to filesystem
				if(result->size>0){
					bakFile=apr_pstrcat(tp,globals->homeDir,"/",resource,".part",NULL);
					status=apr_file_open(&file,bakFile,APR_WRITE|APR_CREATE|APR_TRUNCATE,APR_OS_DEFAULT,tp);
					if(apr_file_write_full(file,result->data,result->size,&file_written)==APR_SUCCESS){
						filename=apr_pstrcat(p,globals->homeDir,"/",resource,NULL);
						apr_file_close(file);
						if(apr_file_rename(bakFile,filename,tp)==APR_SUCCESS){
							ret=filename;
						}
					}else{				
						apr_file_close(file);
						if(details!=NULL){
							*details=apr_pstrcat(p,"Failure to write file:",SAFESTR(bakFile),NULL);
						}
					}
				}
			}else{
				if(details!=NULL){
					*details=apr_pstrcat(p,"Failure to write file (Response Code!=200): ",SAFESTR(resource),",",SAFESTR(errorMsg),NULL);
				}
			}
			
			apr_pool_destroy(tp);
		}
		
		return ret;
	}
	
	char* cc_getLocalResourcePath(pool* p, cc_globals* globals,char* resource,char**details){
		return apr_pstrcat(p,globals->homeDir,"/",resource,NULL);
	}
#endif
