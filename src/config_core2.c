#include <apache_typedefs.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include "config_core2.h"
#include <xml_core.h>
#include <common_utils.h>
#include <db_core2.h>
#include "log_core.h"

#define SERVICE_SYNC_WAIT	30

	config_core* cc_newConfigCoreObj(pool* p){
		config_core* ret=(config_core*)apr_palloc(p,sizeof(config_core));
		ret->globals=cc_newGlobalsObj(p);
		ret->services=apr_array_make (p,4,sizeof(cc_service_descriptor*));
		ret->sheapMapFile=apr_psprintf(p,"/%d.shm", getpid());
		ret->sheapPageSize=0;
		ret->sheap=NULL;
		ret->service_id_configs=apr_hash_make (p);
		ret->service_name_configs=apr_hash_make (p);
		ret->messageBroker=NULL;
		ret->databaseFile=NULL;
		ret->refreshLogFile=NULL;
		ret->disableProcessRecovery=FALSE;
		ret->enableAutoRefresh=FALSE;
		ret->autoRefreshWaitSeconds=20;
		ret->threaded=FALSE;
		return ret;		
	}
	
	
	typedef struct cc_bundle{
		config_core* cc;
		void* userdata, *userdata2;
	}cc_bundle;
	
	static int cc_setCCAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		int i;
		cc_bundle* bundle=(cc_bundle*)userdata;
		for (i = 0; attributes[i]; i += 2) {
			if(strcmp(attributes[i],"namespace")==0){
				bundle->cc->globals->nameSpace=apr_pstrdup(p,attributes[i + 1]);
			} 		
  		}
		return 1;
	}
	static int cc_setCCDataBaseFileBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		int len;
		cc_bundle* bundle=(cc_bundle*)userdata;
		
		bundle->cc->databaseFile=apr_pstrdup(p,body);
		return 1;
	}
	
	static int cc_setCCHomeDirBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		int len;
		cc_bundle* bundle=(cc_bundle*)userdata;
		
		bundle->cc->globals->homeDir=apr_pstrdup(p,body);
		bundle->cc->globals->logsDir=apr_pstrcat(p,body,"/logs",NULL);
		return 1;
	}
	
	static int cc_setCCSheapMapFileBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		bundle->cc->sheapMapFile=apr_pstrdup(p,body);
		return 1;
	}
	static int cc_setCCSheapPageSizeBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		bundle->cc->sheapPageSize=atoi(body);
		return 1;
	}
	static int cc_setCCServiceAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		int i;
		cc_service_descriptor* svc=NULL;
		cc_bundle* bundle=(cc_bundle*)userdata;
		bundle->userdata=NULL;
		svc=(void*)cc_newServiceDescriptorObj(p);
		
		for (i = 0; attributes[i]; i += 2) {
			if(strcmp(attributes[i],"id")==0){
				svc->id=apr_pstrdup(p,attributes[i + 1]);
			}
			if(strcmp(attributes[i],"name")==0){
				svc->name=apr_pstrdup(p,attributes[i + 1]);
			} 	
  		}
  		if(svc->id!=NULL){
  			bundle->userdata=(void*)svc;	
  		}
  		return 1;
	}
	static int cc_setCCServiceUriBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=(cc_service_descriptor*)bundle->userdata;
		if(svc!=NULL){
			svc->uri=apr_pstrdup(p,body);
		}
		return 1;
	}
	static int cc_setCCServiceTimeoutBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=(cc_service_descriptor*)bundle->userdata;
		if(svc!=NULL){
			svc->timeoutSeconds=atol(body);
		}
		return 1;
	}
	static int cc_setCCServiceParamAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		int i;
		cc_bundle* bundle=(cc_bundle*)userdata;
		bundle->userdata2=NULL;
		for (i = 0; attributes[i]; i += 2) {
			if(strcmp(attributes[i],"name")==0){
				bundle->userdata2=(void*)apr_pstrdup(p,attributes[i + 1]);
			} 		
  		}
		return 1;
	}
	static int cc_setCCServiceParamBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=(cc_service_descriptor*)bundle->userdata;
		char* pname=(char*)bundle->userdata2;
		
		if(svc!=NULL&&pname!=NULL){
			apr_hash_set(svc->params,pname,APR_HASH_KEY_STRING,apr_pstrdup(p,body));
			pname=NULL;
		}
		return 1;
	}
	static int cc_setCCServiceEnd(pool* p,char* xPath,int type,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=(cc_service_descriptor*)bundle->userdata;
		cc_service_descriptor** svcplace=NULL;
		if(svc->id!=NULL){
			svcplace=(cc_service_descriptor**)apr_array_push(bundle->cc->services);
			*svcplace=svc;
		}
		
		bundle->userdata=NULL;
		bundle->userdata2=NULL;
		return 1;
	}
	
	static int cc_setResourceServiceAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		int i;
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=NULL;
		
		svc=bundle->cc->globals->resourceService=cc_newServiceDescriptorObj(p);
		svc->id=apr_pstrdup(p,"RESOURCE_SERVICE");
		svc->name=apr_pstrdup(p,"RESOURCE_SERVICE");
  		return 1;
	}
	static int cc_setResourceServiceUriBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=bundle->cc->globals->resourceService;
		if(svc!=NULL){
			svc->uri=apr_pstrdup(p,body);
		}
		return 1;
	}
	static int cc_setResourceServiceParamAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		int i;
		cc_bundle* bundle=(cc_bundle*)userdata;
		bundle->userdata2=NULL;
		for (i = 0; attributes[i]; i += 2) {
			if(strcmp(attributes[i],"name")==0){
				bundle->userdata2=(void*)apr_pstrdup(p,attributes[i + 1]);
			} 		
  		}
		return 1;
	}
	static int cc_setResourceServiceParamBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=bundle->cc->globals->resourceService;
		char* pname=(char*)bundle->userdata2;
		
		if(svc!=NULL&&pname!=NULL){
			apr_hash_set(svc->params,pname,APR_HASH_KEY_STRING,apr_pstrdup(p,body));
			pname=NULL;
		}
		return 1;
	}
	static int cc_setResourceServiceTimeoutBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=bundle->cc->globals->resourceService;
		if(svc!=NULL){
			svc->timeoutSeconds=atol(body);
		}
		return 1;
	}
	static int cc_setResourceServiceUserColonPassBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cc_service_descriptor* svc=bundle->cc->globals->resourceService;
		if(svc!=NULL){
			svc->userColonPass=apr_pstrdup(p,body);
		}
		return 1;
	}
	static int cc_setMessageBrokerAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		int i;
		cc_bundle* bundle=(cc_bundle*)userdata;
		//cm_broker* broker=NULL;
		bundle->cc->messageBroker=cm_newMessageBrokerObj(p);
  		return 1;
	}
	static int cc_setMessageBrokerHostBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cm_broker* broker=bundle->cc->messageBroker;
		if(broker!=NULL){
			broker->host=apr_pstrdup(p,body);
		}
		return 1;
	}
	static int cc_setMessageBrokerPortBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cm_broker* broker=bundle->cc->messageBroker;
		if(broker!=NULL){
			broker->port=atoi(body);
		}
		return 1;
	}
	static int cc_setMessageBrokerUsernameBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cm_broker* broker=bundle->cc->messageBroker;
		if(broker!=NULL){
			broker->username=apr_pstrdup(p,body);
		}
		return 1;
	}
	static int cc_setMessageBrokerPasswordBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cm_broker* broker=bundle->cc->messageBroker;
		if(broker!=NULL){
			broker->password=apr_pstrdup(p,body);
		}
		return 1;
	}
	
	static int cc_setMessageBrokerReconnectWaitMillisBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cc_bundle* bundle=(cc_bundle*)userdata;
		cm_broker* broker=bundle->cc->messageBroker;
		if(broker!=NULL){
			broker->reconnectWaitMs=atol(body);
		}
		return 1;
	}
	static int cc_setMessageBrokerWaitTimeoutSecondsBody(pool* p,char* xPath,int type,const char *body,void* userdata){
			cc_bundle* bundle=(cc_bundle*)userdata;
			cm_broker* broker=bundle->cc->messageBroker;
			if(broker!=NULL){
				broker->waitTimeoutSeconds=atol(body);
			}
			return 1;
		}
	static int cc_setCCDisableProcessRecoveryBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		int len;
		cc_bundle* bundle=(cc_bundle*)userdata;
		
		bundle->cc->disableProcessRecovery=STRTOBOOL(body);
		return 1;
	}

	static int cc_setCCEnableAutoRefreshBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		int len;
		cc_bundle* bundle=(cc_bundle*)userdata;

		bundle->cc->enableAutoRefresh=STRTOBOOL(body);
		return 1;
	}

	static int cc_setCCEnableThreadedBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		int len;
		cc_bundle* bundle=(cc_bundle*)userdata;

		bundle->cc->threaded=STRTOBOOL(body);
		return 1;
	}

	static int cc_setCCAutoRefreshWaitSecondsBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		int len;
		cc_bundle* bundle=(cc_bundle*)userdata;
		if(body!=NULL) {
			bundle->cc->autoRefreshWaitSeconds=atol(body);
		}
		return 1;
	}

	char* cc_loadConfigCoreFile(pool* p, char* file, config_core* conf){
		char* result=NULL;
		XmlCore* xCore;
		
		cc_bundle* bundle=(cc_bundle*)apr_palloc(p,sizeof(cc_bundle));
		bundle->userdata=NULL;
		bundle->userdata2=NULL;
		bundle->cc=conf;
		
		xCore=xmlcore_getXmlCore(p);
		xmlcore_addXPathHandler(xCore,"/config-core",0,cc_setCCAttributes,NULL,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/homeDir",0,NULL,cc_setCCHomeDirBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/sheapMapFile",0,NULL,cc_setCCSheapMapFileBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/sheapPageSize",0,NULL,cc_setCCSheapPageSizeBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/databaseFile",0,NULL,cc_setCCDataBaseFileBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/disableProcessRecovery",0,NULL,cc_setCCDisableProcessRecoveryBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/enableAutoRefresh",0,NULL,cc_setCCEnableAutoRefreshBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/autoRefreshWaitSeconds",0,NULL,cc_setCCAutoRefreshWaitSecondsBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/threaded",0,NULL,cc_setCCEnableThreadedBody,NULL, bundle);
		
		xmlcore_addXPathHandler(xCore,"/config-core/messageBroker",0,cc_setMessageBrokerAttributes,NULL,NULL, bundle);		
		xmlcore_addXPathHandler(xCore,"/config-core/messageBroker/host",0,NULL,cc_setMessageBrokerHostBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/messageBroker/port",0,NULL,cc_setMessageBrokerPortBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/messageBroker/username",0,NULL,cc_setMessageBrokerUsernameBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/messageBroker/password",0,NULL,cc_setMessageBrokerPasswordBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/messageBroker/reconnectWaitMillis",0,NULL,cc_setMessageBrokerReconnectWaitMillisBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/messageBroker/waitTimeSeconds",0,NULL,cc_setMessageBrokerWaitTimeoutSecondsBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/resourceService",0,cc_setResourceServiceAttributes,NULL,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/resourceService/uri",0,NULL,cc_setResourceServiceUriBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/resourceService/timeoutSeconds",0,NULL,cc_setResourceServiceTimeoutBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/resourceService/userColonPass",0,NULL,cc_setResourceServiceUserColonPassBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/resourceService/param",0,cc_setResourceServiceParamAttributes,cc_setResourceServiceTimeoutBody,NULL, bundle);
		
		xmlcore_addXPathHandler(xCore,"/config-core/services/service",0,cc_setCCServiceAttributes,NULL,cc_setCCServiceEnd, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/services/service/uri",0,NULL,cc_setCCServiceUriBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/services/service/timeoutSeconds",0,NULL,cc_setCCServiceTimeoutBody,NULL, bundle);
		xmlcore_addXPathHandler(xCore,"/config-core/services/service/param",0,cc_setCCServiceParamAttributes,cc_setCCServiceParamBody,NULL, bundle);
		result=xmlcore_beginParsingTextResponse(xCore,file);
		
		
		return result;	
	}
	
	service_module_template* cc_findServiceTemplate(char* id){
		int x;
		int len=sizeof(cc_service_module_templates)/sizeof(service_module_template);
		if(id!=NULL){
			for(x=0;x<len;x++){
				if(cc_service_module_templates[x].id!=NULL&&strcmp(cc_service_module_templates[x].id,id)==0){
					return (service_module_template*)&(cc_service_module_templates[x]);
				}
			}
		}
		return NULL;
	}
	static int cc_onSheapPageFlip(pool* p, shared_heap* sheap, void* userdata){
		config_core* configCore=(config_core*)userdata;
		int i=0;
		service_module_template* serviceModule=NULL;
		cc_service_descriptor* svc=NULL;
		char* error=NULL;
		void* moduleConfig=NULL;
		
//		configCore->alertsCore=alertscore_fetchFromSheap(sheap);
//		configCore->diagCore=diagcore_fetchFromSheap(sheap);
//		configCore->auditCore=auditcore_fetchFromSheap(sheap);
//		servicemon_syncSelf(p,configCore->serviceMonitor);
		
		
		for(i=0;i<configCore->services->nelts;i++){
			svc=(cc_service_descriptor*)getElement(configCore->services,i);
			serviceModule=cc_findServiceTemplate(svc->id);
			if(serviceModule!=NULL&&serviceModule->postInit_func!=NULL){
				error=(*serviceModule->postInit_func)(p,configCore->sheap,configCore->globals,svc,&moduleConfig);
				if(error==NULL){
					//set as post initialized
					if(moduleConfig!=NULL){
						apr_hash_set(configCore->service_id_configs,svc->id,APR_HASH_KEY_STRING,moduleConfig);
						
						if(svc->name!=NULL){
							apr_hash_set(configCore->service_name_configs,svc->name,APR_HASH_KEY_STRING,moduleConfig);
						}
					}
				}
			}
		}
		return 1;
	}
	int cc_syncSelf(apr_pool_t* pool,config_core* configCore){
		int ret=0;
		if(configCore!=NULL&&(ret=shmdata_syncself(pool,configCore->sheap,cc_onSheapPageFlip,configCore))==2){
			return 1;	
		}
		return 0;
	}
	
	static void* cc_initMessaging(pool* p, void* userdata){
		config_core* configCore=(config_core*)userdata;
		database_core* dbCore=NULL;
		
		dbCore=dbcore_bindToDatabaseCore(p,configCore->databaseFile);
		
		return (void*)dbCore;
	}
	
	static char* cc_refreshConfigCore(pool* p,database_core* dbLogCore,config_core* conf){
		int i;
		service_module_template* serviceModule=NULL;
		cc_service_descriptor* svc=NULL;
		void* moduleConfig=NULL;
		char* error=NULL;
//		alerts_core* alertsCore=NULL;
//		diagnostics_core* diagCore=NULL;	
		
		char cbuf[APR_CTIME_LEN + 1];
		long timeTakenMillis;
		
		logcore_openLogFile(p,conf->refreshLogFile);
		logcore_truncateLogFile();
		
		apr_time_t t1 = apr_time_now();
		apr_ctime(cbuf, t1);
		logcore_printLog("\n°\t Refreshing config [%s]\n", cbuf); fflush(stdout);
		
		if(conf->services->nelts>0){
			shmdata_BeginTagging(conf->sheap);
						
			for(i=0;i<conf->services->nelts;i++){
				svc=(cc_service_descriptor*)getElement(conf->services,i);
				serviceModule=cc_findServiceTemplate(svc->id);
				if(serviceModule!=NULL){
					if(serviceModule->refresh_func!=NULL){
						error=(*serviceModule->refresh_func)(p,conf->sheap,conf->globals,svc,&moduleConfig);
						if(error!=NULL){
							dbcore_logError(p,dbLogCore,serviceModule->id,"REFRESH",error);
							logcore_closeLogFile();
							return apr_pstrcat(p,"[",serviceModule->id,"] Service failed to refresh: ",error,NULL);	
						}else{
							//set as initialized
							if(moduleConfig!=NULL){
								apr_hash_set(conf->service_id_configs,svc->id,APR_HASH_KEY_STRING,moduleConfig);
								
								if(svc->name!=NULL){
									apr_hash_set(conf->service_name_configs,svc->name,APR_HASH_KEY_STRING,moduleConfig);
								}
							}
						}
					}
				}else{
					logcore_closeLogFile();
					return apr_pstrcat(p,"Service: \"",svc->id,"\" not found",NULL); 	
				}
			}
			//publish bash segment
			if(error==NULL){
				shmdata_PublishBackSeg(conf->sheap);
				cc_syncSelf(p,conf);
			}
		}

		timeTakenMillis = ((apr_time_now() - t1) / 1000);
		logcore_printLog("\n°\t Refresh complete [time taken : %d milliseconds]\n", timeTakenMillis); fflush(stdout);
		logcore_closeLogFile();
		
		return error;	
	}
	static char* cc_handleMessageRecieved(pool* p, cm_connection* cmConn,cm_wire_message* msg,void* localConfig, void* userdata, int autoRefresh){
		char* result=NULL;
		config_core* configCore=(config_core*)userdata;
		database_core* dbCore=(database_core*)localConfig;
		
		dbcore_registerMessage(p,dbCore,msg);
		
		//now we can do out actions that we would like to do...like refresh the config core:)
		if(cm_isRefreshMessage(msg)){
			result=cc_refreshConfigCore(p,dbCore,configCore);
			if(result==NULL){
				if(autoRefresh){
					logcore_printLog("Auto Refresh succeded\n"); fflush(stdout);
				}else{
					cm_sendRefreshComplete(p,cmConn,msg->batchId);
				}
			}else{
				if(autoRefresh) {
					logcore_printLog("Auto Refresh failed:%s\n", result); fflush(stdout);
				}else{
					cm_sendRefreshFailed(p,cmConn,result,msg->batchId);
				}
			}
		}
		return NULL;
	}
	
	char* cc_initializeConfigCore(pool* p,config_core* conf){
		int i=0;
		service_module_template* serviceModule=NULL;
		cc_service_descriptor* svc=NULL;
		char* error=NULL;
		char* sheapfile=apr_pstrcat(p,conf->globals->homeDir,conf->sheapMapFile,NULL);
		void* moduleConfig=NULL;
		char* errorRet=NULL;
		database_core* dbCore=NULL;

		logcore_printLog("° Initializing Config Core \n\r");
		logcore_printLog("\tSetup filesystem");
		cc_initGlobals(p,conf->globals);
		
		
		logcore_printLog("\tSheapFile:\t%s\n",sheapfile);
		logcore_printLog("\tSheapPageSize:\t%d\n",conf->sheapPageSize);
		conf->sheap=shmdata_sheap_make(p, conf->sheapPageSize,sheapfile);
		if(conf->sheap==NULL){
			return apr_pstrdup(p,"Could not create Config Core shared heap");	
		}
		shmdata_BeginTagging(conf->sheap);
		
		
		//expanding resourceServiceUri
		if(conf->globals->resourceService!=NULL&&conf->globals->resourceService->uri!=NULL){
			if(conf->globals->nameSpace!=NULL){
				conf->globals->resourceService->uri=apr_pstrcat(p,conf->globals->resourceService->uri,"/",conf->globals->nameSpace,NULL);
			}	
		}
		
		//expand databaseFile path
		if(conf->databaseFile!=NULL){
			conf->databaseFile=apr_pstrcat(p,conf->globals->homeDir,conf->databaseFile,NULL);
		}
		
		
		logcore_printLog("»\t Verifying Realm Database...");
		fflush(stdout);
		if((error=dbcore_initializeRealmDatabase(p,conf->databaseFile))!=NULL){
			logcore_printLog("FAILURE\r\n");
			return apr_pstrcat(p,"Could not initialize realm database:",error,NULL);
		}else{
			logcore_printLog("OK\r\n");
		}
		logcore_printLog("\n");fflush(stdout);
		
		dbCore=dbcore_bindToDatabaseCore(p,conf->databaseFile);
		if(dbCore==NULL){
			return apr_pstrcat(p,"Could not bind to realm database:",error,NULL);	
		}
		
		//load modules
		
		if(conf->services->nelts>0){
			logcore_printLog("°\tServices Init(%d)-\n",conf->services->nelts);
			for(i=0;i<conf->services->nelts;i++){
				svc=(cc_service_descriptor*)getElement(conf->services,i);
				serviceModule=cc_findServiceTemplate(svc->id);
				if(serviceModule!=NULL){
					if(svc->name!=NULL){
						logcore_printLog("\t°\t%s [%s] Initializing...\n",serviceModule->id, SAFESTRBLANK(svc->name));
					}else{
						logcore_printLog("\t°\t%s Initializing...\n",serviceModule->id);
					}
					fflush(stdout);
					if(serviceModule->init_func!=NULL){
						error=(*serviceModule->init_func)(p,conf->sheap,conf->globals,svc,&moduleConfig);
						if(error!=NULL){
							if(errorRet==NULL){
								errorRet=apr_pstrdup(p,"• ");
							}else{
								errorRet=apr_pstrcat(p,errorRet,"\r\n• ",NULL);
							}
							dbcore_logError(p,dbCore,serviceModule->id,"INIT",error);
							errorRet=apr_pstrcat(p,errorRet,"[",serviceModule->id,"] Service failed to initialize: ",error,NULL);	
						}else{
							//set as initialized
							if(moduleConfig!=NULL){
								//set config by id
								apr_hash_set(conf->service_id_configs,svc->id,APR_HASH_KEY_STRING,moduleConfig);
								
								//set config by name
								if(svc->name!=NULL){
									apr_hash_set(conf->service_name_configs,svc->name,APR_HASH_KEY_STRING,moduleConfig);
								}
							}
						}
					}
				}else{
					return apr_pstrcat(p,"Service: \"",svc->id,"\" not found",NULL); 	
				}
				fflush(stdout);
			}
			
			//publish bash segment
			logcore_printLog("»\tPublishing Config Core Sheap...");
			fflush(stdout);
			shmdata_PublishBackSeg(conf->sheap);
			logcore_printLog("OK\r\n");
			cc_syncSelf(p,conf);
			
			if(conf->messageBroker!=NULL){
				logcore_printLog("»\t Testing Message Broker...");
				fflush(stdout);
				if((error=cm_testMessageBroker(p,conf->globals->nameSpace,conf->messageBroker))!=NULL){
					logcore_printLog("FAILURE: %s\r\n",error);
				}else{
					logcore_printLog("OK\r\n");	
				}
			}
			
			//close database connection so message loop can pick it up
			dbcore_close(p,dbCore);
			
			
			
			
			logcore_printLog("°\tStarting Config Core Realm Process:\n");
			logcore_closeLogFile();//closing before forking
			cm_initializeMessagingLoop(p, conf->globals->nameSpace,conf->globals->logsDir,conf->messageBroker, conf,cc_initMessaging,cc_handleMessageRecieved,conf->disableProcessRecovery,conf->threaded);
			logcore_openLogFile(p,conf->refreshLogFile);
			
			
		}
		return errorRet;
	}
	
	static void cc_printServiceDescriptor(pool* p, cc_service_descriptor*svc){
		apr_hash_index_t *hi;
		char *name,*val;
		if(svc!=NULL){
			if(svc->name!=NULL){
				logcore_printLog("\t•\t%s [%s]\n",svc->id,svc->name);	
			}else{
				logcore_printLog("\t•\t%s\n",svc->id);
			}
			logcore_printLog("\t\tUri:\t%s\n",svc->uri);
			logcore_printLog("\t\tTimeoutSeconds:\t%d\n",svc->timeoutSeconds);
			for (hi = apr_hash_first(p, svc->params); hi; hi = apr_hash_next(hi)) {
				apr_hash_this(hi,(const void**)&name, NULL, (void**)&val);
				logcore_printLog("\t\t%s:\t%s\n",name,val);
			}
			logcore_printLog("\r\n");
		}else{
			logcore_printLog("!!! SERVICE IS NULL\n");	
		}
	}
	void cc_printConfigCoreDetails(pool* p,config_core* conf){
//		apr_hash_index_t *hi;
		cc_service_descriptor* svc=NULL;
//		char *name,*val;
		int i=0;
		if(conf==NULL){
			logcore_printLog("Config-Core: NULL!!\n");
			return;
		}
		logcore_printLog("Config-Core:\n");
		logcore_printLog("•\tGlobals-\n");
		logcore_printLog("\t\tNameSpace:%s\n",conf->globals->nameSpace);
		logcore_printLog("\t\tHomeDir:%s\n",conf->globals->homeDir);
		logcore_printLog("\t\tEnableAutoRefresh:%d\n",conf->enableAutoRefresh);
		logcore_printLog("\t\tThreaded:%d\n",conf->threaded);
		
		logcore_printLog("•\tSystem Services-\n");
		if(conf->messageBroker!=NULL){
			logcore_printLog("\t•\tMessageBroker:\n");
			logcore_printLog("\t\t\tHost:%s\n",conf->messageBroker->host);
			logcore_printLog("\t\t\tPort:%d\n",conf->messageBroker->port);
			if(conf->messageBroker->username!=NULL){
				logcore_printLog("\t\t\tUsername:%s\n",conf->messageBroker->username);
			}
			if(conf->messageBroker->password!=NULL){
				logcore_printLog("\t\t\tPassword:%s\n",conf->messageBroker->password);	
			}
			logcore_printLog("\t\t\tReconnect Wait(ms):%d\n",conf->messageBroker->reconnectWaitMs);
		}else{
			logcore_printLog("\t•\tMessageBroker: NOT ACTIVATED\n");	
		}
		
		if(conf->globals->resourceService!=NULL){
			cc_printServiceDescriptor(p,conf->globals->resourceService);
		}else{
			logcore_printLog("•\tResourceService: NOT ACTIVATED\n");	
		}
		
		logcore_printLog("\t\tSheapMapFile:%s\n",conf->sheapMapFile);
		logcore_printLog("\t\tSheapPageSize:%d\n",conf->sheapPageSize);
		logcore_printLog("\t\tDatabaseFile:%s\n",conf->databaseFile);
		if(conf->services->nelts>0){
			logcore_printLog("•\tServices-\n");		
			for(i=0;i<conf->services->nelts;i++){
				svc=(cc_service_descriptor*)getElement(conf->services,i);
				cc_printServiceDescriptor(p,svc);
			}
		}
		
	}
	
	void* cc_getModuleConfigByName(config_core* conf,char* name){
		return apr_hash_get(conf->service_name_configs,name,APR_HASH_KEY_STRING);		
	}
	void* cc_getModuleConfigById(config_core* conf,char* id){
		return apr_hash_get(conf->service_id_configs,id,APR_HASH_KEY_STRING);	
	}
