#include <apache_mappings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <common_utils.h>
#include "config_messaging2.h"
#include "config_messaging_parsing2.h"
#include "logging.h"
#include "http_client.h"
#include "config_core2.h"
#include "xml_core.h"

#define MESSAGE_TOPIC	"/topic/LIBSWITCH.REALM"
#define URI_POSTFIX_AUTOREFRESH_TIMESTAMP_XML		"switch-config.xml"
#define HOSTNAME_BUFSIZE 	256

	cm_broker* cm_newMessageBrokerObj(pool* p){
			cm_broker* ret=(cm_broker*)apr_pcalloc(p,sizeof(cm_broker));
			ret->port=0;
			ret->host=NULL;
			ret->username = NULL;
			ret->password=NULL;
			ret->reconnectWaitMs=20000;
			ret->waitTimeoutSeconds=1200;
			return ret;
	}
	
	cm_connection* cm_newConnectionObj(pool* p, char* nameSpace){
			cm_connection* ret=(cm_connection*)apr_palloc(p,sizeof(cm_connection));
			ret->stomp=stompu_newConnectionObj(p);
			ret->wireHeader=cm_newWireHeader(p,nameSpace);
			return ret;
	}
	
	apr_status_t cm_sendMessage(pool* p, cm_connection* cmConn, cm_wire_message* msg){
		apr_status_t rc;
		char* msgSerialized=NULL;
		if(msg==NULL){ return APR_EGENERAL;}
		
		msgSerialized=cm_serializeMessage(p,msg);
		//printf("%s\n",msgSerialized);
		rc=stompu_sendMessage(p,cmConn->stomp,MESSAGE_TOPIC,msgSerialized);
		return rc;
	}
	
	cm_wire_message* cm_getNextWireMessage(pool* p, cm_connection* cmConn){
		cm_wire_message* ret=NULL;
		apr_status_t rc;
		stomp_frame *frame;
		
		if(stomp_connection_isValid(cmConn->stomp)){
			rc = stomp_read(cmConn->stomp, &frame, p);
			if(rc==APR_SUCCESS&& strcmp(frame->command,"MESSAGE")==0){
				ret=cm_parseMessage(p,frame->body);
				return ret;
			}
		}
		return NULL;	
	}
	char* cm_connect(pool* p,cm_connection* cmConn, cm_broker* messageBroker, apr_int32_t timeoutSeconds){
		char* error=NULL;
		error=stompu_connect(p,cmConn->stomp,messageBroker->host, messageBroker->port,messageBroker->username,messageBroker->password,timeoutSeconds*APR_USEC_PER_SEC);
		return error;	
	}
	char* cm_disconnect(pool* p,cm_connection* cmConn){
		char* error=NULL;
		error=stompu_disconnect(p,cmConn->stomp);
		return error;	
	}
	char* cm_newMessageBatchId(pool* p){
		char buf[64];
		int rnd=0;
		srand((unsigned int)time(NULL));
		rnd=rand();
		sprintf(buf,"%d",rnd);
		return apr_pstrdup(p,buf);
	}
	
	char* cm_testMessageBroker(pool* p, char* nameSpace, cm_broker* messageBroker){
		char* result=NULL;
		cm_wire_message* msg=NULL, *msgr=NULL;
		
		apr_status_t rc;
		cm_connection* cmConn=cm_newConnectionObj(p,nameSpace);
		
		
		if(messageBroker==NULL){
			return apr_pstrdup(p,"MessageBroker Object Is NULL");
		}
		printf("connect..");fflush(stdout);
		
		result=cm_connect(p,cmConn,messageBroker,2);
		if(result==NULL){
			printf("send..");fflush(stdout);
			rc=cm_sendAlive(p,cmConn,NULL);
			if(rc!=APR_SUCCESS){
				return apr_pstrcat(p,"Could not send to test message to: ",MESSAGE_TOPIC,NULL);
			}
			//printf("%s\n",cm_serializeMessage(p,msg));
			printf("disconnect..");fflush(stdout);
			stompu_disconnect(p,cmConn->stomp);
		}	
		return result;
	}
	
	static void cm_setNotifyNamespace(pool* p,cm_wire_message* msg,char* ns){
		if(msg!=NULL&&ns!=NULL){
			apr_hash_set(msg->params,"notifyNamespace",APR_HASH_KEY_STRING,apr_pstrdup(p,ns));
		}
	}
	static char* cm_getNotifyNamespace(cm_wire_message* msg){
		if(msg==NULL) return NULL;
		return (char*)apr_hash_get(msg->params,"notifyNamespace",APR_HASH_KEY_STRING);
	}

	apr_status_t cm_sendPong(pool* p, cm_connection* cmConn, char* batchId){
			cm_wire_message* msg=cm_newWireMessageType(p,"PONG",cmConn->wireHeader,batchId);
			return cm_sendMessage(p,cmConn,msg);
	}
	apr_status_t cm_sendPing(pool* p, cm_connection* cmConn,char* ns){
			cm_wire_message* msg=cm_newWireMessageType(p,"PING",cmConn->wireHeader,cm_newMessageBatchId(p));
			cm_setNotifyNamespace(p,msg,ns);
			return cm_sendMessage(p,cmConn,msg);
	}
	apr_status_t cm_sendRefresh(pool* p, cm_connection* cmConn,char* ns){
			cm_wire_message* msg=cm_newWireMessageType(p,"REFRESH",cmConn->wireHeader,cm_newMessageBatchId(p));
			cm_setNotifyNamespace(p,msg,ns);
			return cm_sendMessage(p,cmConn,msg);
	}
	apr_status_t cm_sendAlive(pool* p, cm_connection* cmConn,char* ns){
			cm_wire_message* msg=cm_newWireMessageType(p,"ALIVE",cmConn->wireHeader,cm_newMessageBatchId(p));
			cm_setNotifyNamespace(p,msg,ns);
			return cm_sendMessage(p,cmConn,msg);
	}
	apr_status_t cm_sendRefreshComplete(pool* p, cm_connection* cmConn,char* batchId){
			cm_wire_message* msg=cm_newWireMessageType(p,"REFRESH-COMPLETE",cmConn->wireHeader,batchId);
			return cm_sendMessage(p,cmConn,msg);
	}
	apr_status_t cm_sendRefreshFailed(pool* p, cm_connection* cmConn,char* batchId,char* errors){
			cm_wire_message* msg=cm_newWireMessageType(p,"REFRESH-FAILED",cmConn->wireHeader,batchId);
			if(errors!=NULL){
				apr_hash_set(msg->params,"errors",APR_HASH_KEY_STRING,errors);
			}
			return cm_sendMessage(p,cmConn,msg);
	}
	
	static char* cm_getMessageType(cm_wire_message* msg){
		if(msg!=NULL){
			return msg->type;
		}
		return NULL;
	}
	
	static char* cm_getMessageNamespace(cm_wire_message* msg){
		if(msg!=NULL&&msg->header!=NULL){
			return msg->header->nameSpace;
		}
		return NULL;
	}
	static char* cm_getMessageNodeName(cm_wire_message* msg){
		if(msg!=NULL&&msg->header!=NULL){
			return msg->header->nodeName;
		}
		return NULL;
	}
	int cm_isAliveMessage(cm_wire_message* msg){
		return (msg!=NULL&&strcmp(msg->type,"ALIVE")==0);	
	}
	int cm_isPingMessage(cm_wire_message* msg){
		return (msg!=NULL&&strcmp(msg->type,"PING")==0);	
	}
	int cm_isRefreshMessage(cm_wire_message* msg){
		return (msg!=NULL&&(strcmp(msg->type,"REFRESH")==0||strcmp(msg->type,"AUTO-REFRESH")==0));
	}
	int cm_isRefreshCompleteMessage(cm_wire_message* msg){
		return (msg!=NULL&&strcmp(msg->type,"REFRESH-COMPLETE")==0);	
	}
	int cm_isRefreshFailedMessage(cm_wire_message* msg){
		return (msg!=NULL&&strcmp(msg->type,"REFRESH-FAILED")==0);	
	}

	static cm_wire_message* cm_generateAutoRefreshMessage(pool* p, char* namespace){
		cm_wire_header* wireHeader=cm_newWireHeader(p,namespace);
		cm_wire_message* msg=cm_newWireMessageType(p,"AUTO-REFRESH",wireHeader,cm_newMessageBatchId(p));
		apr_hash_set(msg->params,"notifyNamespace",APR_HASH_KEY_STRING,apr_pstrdup(p,namespace));
		return msg;
	}

	typedef struct next_refresh_info{
		time_t nextRefreshTimestamp;
		char*  nextRefreshServerRegex;
	}next_refresh_info;

	static int cm_setRefreshTimestampAttribute(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		int i;
		next_refresh_info* refreshInfo=(next_refresh_info*)userdata;
		for(i=0;attributes[i]; i += 2) {
			if(strcmp(attributes[i],"nextRefreshTime")==0){
				refreshInfo->nextRefreshTimestamp=comu_dateStringToSeconds((char*)attributes[i + 1]);
			} else if(strcmp(attributes[i],"nextRefreshServerRegex")==0){
				refreshInfo->nextRefreshServerRegex=apr_pstrdup(p, (char*)attributes[i + 1]);
			}
		}
		return 1;
	}

	static int cm_canAutoRefreshNow(pool* p,cc_service_descriptor* resourceService, const time_t lastRefreshTimestamp, char* namespace,mm_logger* logger){
		http_util_result* httpResult=NULL;
		char* reqQuery=NULL;
		char* error = NULL;
		time_t currentTimestamp;
		XmlCore* xCore=NULL;
		char* result=NULL;

		if(resourceService==NULL) return FALSE;

		currentTimestamp = time(NULL);

		reqQuery=apr_pstrcat(p,resourceService->uri,"/",URI_POSTFIX_AUTOREFRESH_TIMESTAMP_XML,NULL);

		//mmlog_log(logger, "reqQuery : %s", reqQuery);

		httpResult=http_request_get_verbose2(p,reqQuery,resourceService->timeoutSeconds,5,resourceService->userColonPass,NULL,&error);

		if(httpResult==NULL||httpResult->data==NULL) {
			mmlog_log(logger,"AutoRefresh httpResult Error: %s", (error!=NULL) ? error : "unknown");
			return FALSE;
		}

		if(!http_is200_OK(httpResult)) {
			mmlog_log(logger,"AutoRefresh httpResult Error[%d]: %s", httpResult->responseCode, (error!=NULL) ? error : "unknown");
			return FALSE;
		}

		next_refresh_info refreshInfo;
		refreshInfo.nextRefreshTimestamp = -1;
		refreshInfo.nextRefreshServerRegex = NULL;

		xCore=xmlcore_getXmlCore(p);

		xmlcore_addXPathHandler(xCore,"/switch-config",0,cm_setRefreshTimestampAttribute,NULL,NULL, &refreshInfo);

		result=xmlcore_parseFromStringSourceTextResponse(xCore, httpResult->data);

		if(result!=NULL) {
			mmlog_log(logger,"AutoRefreshTimestamp error : %s", result);
			return FALSE;
		}

		if( (lastRefreshTimestamp<refreshInfo.nextRefreshTimestamp) && (refreshInfo.nextRefreshTimestamp<=currentTimestamp)) {
			char hostNameBuf[HOSTNAME_BUFSIZE];

			// check the server regex.
			if(refreshInfo.nextRefreshServerRegex==NULL || gethostname(hostNameBuf, HOSTNAME_BUFSIZE)<0 ) { // nothing to match
				return TRUE;
			}

			// matched with host
			if(switchconf_matchByStrings(p, refreshInfo.nextRefreshServerRegex, (char*)hostNameBuf)==0){
				return TRUE;
			}else{
				mmlog_log(logger,"AutoRefresh ignored, nextRefreshServerRegex : %s and hostNameBuf:%s didn't match", refreshInfo.nextRefreshServerRegex, hostNameBuf);
				return FALSE;
			}
		}

		mmlog_log(logger,"AutoRefresh ignored, nextRefreshTimestamp : %d is tool old or already processed, lastRefreshTimestamp:%d, currentTimestamp:%d", refreshInfo.nextRefreshTimestamp, lastRefreshTimestamp, currentTimestamp);
		return FALSE;
	}

	static void cm_internalMessagingLoop(pool* p, char* nameSpace,char* logsDir, cm_broker* messageBroker,void* localConfig, void* userdata,cm_message_recieved_func msgRecFunc){
		cm_connection* cmConn=cm_newConnectionObj(p,nameSpace);
		cm_wire_message* msg=NULL;
		char* error=NULL;
		apr_status_t rc;
		apr_time_t 	lastHello=0;
		char errorbuf[512];
		
		char* notifyNamespace=NULL;
		time_t lastAutoRefreshTimestamp = time(NULL); // startup time
		
		sprintf(errorbuf,"%s/refresh.log",logsDir);
		mm_logger* logger=mmlog_getLogger(p,errorbuf, 2);
		
		cc_service_descriptor* resourceService=((config_core*)userdata)->globals->resourceService;
		int enableAutoRefresh = ((config_core*)userdata)->enableAutoRefresh;
		int autoRefreshWaitSeconds = ((config_core*)userdata)->autoRefreshWaitSeconds;

		//start wait for message loop
		while(1){
			if(!enableAutoRefresh) {
				if(!stomp_connection_isValid(cmConn->stomp)){ //ensure connection is valid
					mmlog_log(logger,"Connecting to JMS");
					error=cm_connect(p,cmConn,messageBroker,messageBroker->waitTimeoutSeconds);
					if(error==NULL){
						mmlog_log(logger,"Subscribing to %s",MESSAGE_TOPIC);
						if((rc=stompu_subscribe(p,cmConn->stomp,MESSAGE_TOPIC))==APR_SUCCESS){
							mmlog_log(logger,"Sending Ping:)");

							rc=cm_sendPing(p,cmConn,NULL);
							if(rc==APR_SUCCESS){
								lastHello=apr_time_now();
								mmlog_log(logger,"Ping Successful");
							}else{
								mmlog_log(logger,"Ping failed");
							}
						}else{
							mmlog_log(logger,"Subscribe failed");
						}
					}else{
						mmlog_log(logger,"Connect failed: %s",	SAFESTR(error));
					}
				}
				if(!stomp_connection_isValid(cmConn->stomp)){ //if still connection bad, set timeout to return
					//fprintf(stderr,"Sleep begin");
					usleep((messageBroker->reconnectWaitMs)*1000);
					//fprintf(stderr,"Sleep end");
				}else{
					//printf("waiting for message:\r\n");
					//wait for message
					//fprintf(stderr,"msg wait");
					msg=cm_getNextWireMessage(p,cmConn);
					if(msg!=NULL){
						mmlog_log(logger,"Recieved message: %s (%s: %s)",SAFESTR(cm_getMessageType(msg)),SAFESTR(cm_getMessageNodeName(msg)),SAFESTR(cm_getMessageNamespace(msg)));
						notifyNamespace=cm_getNotifyNamespace(msg);
						//internal hello handling
						if(notifyNamespace==NULL||strcmp(notifyNamespace,nameSpace)==0){
							if(cm_isPingMessage(msg)){
								cm_sendPong(p,cmConn,msg->batchId);
							}
							if(msgRecFunc!=NULL){
								(*msgRecFunc)(p,cmConn,msg,localConfig,userdata,FALSE);
							}
						}
					}else{
						if((rc=cm_sendAlive(p,cmConn,NULL))!=APR_SUCCESS){
							apr_strerror(rc,errorbuf,256);
							mmlog_log(logger,"JMS Connectivity failed: %s",SAFESTR(errorbuf));
						}
					}
					//printf("end wait for message\r\n");
				}
			} else { 			// autorefresh
				mmlog_log(logger,"Checking AutoRefresh...");
				if(cm_canAutoRefreshNow(p, resourceService, lastAutoRefreshTimestamp, nameSpace, logger)){
					msg=cm_generateAutoRefreshMessage(p,nameSpace);
					if(msg!=NULL){
						mmlog_log(logger,"AutoRefresh processing message: %s (%s: %s)",cm_getMessageType(msg),cm_getMessageNodeName(msg),cm_getMessageNamespace(msg));
						//internal hello handling
						if(msgRecFunc!=NULL){
							(*msgRecFunc)(p,NULL,msg,localConfig,userdata,TRUE);
						}
						mmlog_log(logger,"AutoRefresh successful");
						lastAutoRefreshTimestamp = time(NULL); // update with current timestamp;
					}
				}
				usleep(autoRefreshWaitSeconds*1000*1000);
			}

		}
		
			
		
	}
	
	typedef struct messaging_proc_rec{
		pool* p;
		char* nameSpace;
		char* logDir;
		cm_broker* messageBroker;
		void* userdata;
		cm_init_messaging_func initFunc;
		cm_message_recieved_func msgRecFunc;		
	}messaging_proc_rec;
	
	static messaging_proc_rec*cm_newMessagingProcRecObj(pool* p, char* nameSpace, char* logDir, 
		cm_broker* messageBroker, void* userdata, cm_init_messaging_func initFunc, 
		cm_message_recieved_func msgRecFunc){
		messaging_proc_rec* mpr=(messaging_proc_rec*)apr_palloc(p,sizeof(messaging_proc_rec));
		mpr->p = p;
		mpr->nameSpace = nameSpace;
		mpr->logDir = logDir;
		mpr->messageBroker = messageBroker;
		mpr->userdata = userdata;
		mpr->initFunc = initFunc;
		mpr->msgRecFunc = msgRecFunc;
		return mpr;
	}
	
	static messaging_proc_rec* root_mpr = NULL;	// Message created by root process
	void cm_startMessagingProcess(pool* p, apr_proc_t* proc, messaging_proc_rec*msg,int disableProcessRecovery);

#if APR_HAS_OTHER_CHILD	&& MSG_PROC_RESTART
	static void cm_messageProcessRestartCallback(int reason, void *data, apr_wait_t status) {
		apr_proc_t *proc = data;
		int mpm_state;
		int stopping;

		switch (reason) {
			case APR_OC_REASON_DEATH:
				apr_proc_other_child_unregister(data);
				/* If apache is not terminating or restarting,
				 * restart the message processer
				 */
				stopping = 1; 	/* if MPM doesn't support query,
	                           	 * assume we shouldn't restart message process
	                             */
				//if (ap_mpm_query(AP_MPMQ_MPM_STATE, &mpm_state) == APR_SUCCESS &&
				//	mpm_state != AP_MPMQ_STOPPING) {
				//	stopping = 0;
				//}
				if (1) {
					//ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL, "message processer died, restarting");
					if(root_mpr!=NULL){
						cm_startMessagingProcess(root_mpr->p, proc, root_mpr,FALSE);
					}
				}
				break;
              
			case APR_OC_REASON_RESTART:
				/* don't do anything; server is stopping or restarting */
				apr_proc_other_child_unregister(data);
				break;
               
			case APR_OC_REASON_LOST:
				/* Restart the child messaging processor as we lost it */
				apr_proc_other_child_unregister(data);
				//ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL, "we lost messaging process, restarting");
				//cm_startMessageProcess(root_mpr->p, proc, root_mpr);
              			break;
              
			case APR_OC_REASON_UNREGISTER:
				/* we get here when child message processor is cleaned up; it gets cleaned
				 * up when pconf gets cleaned up
				 */
				if(proc!=NULL){
					kill(proc->pid, SIGHUP); /* send signal to message processor to die */
				}
				break;
       }
   }
#endif

#ifndef WIN32	
	void cm_startMessagingProcess(pool* p, apr_proc_t* proc, messaging_proc_rec*mpr, int disableProcessRecovery){
		apr_status_t rv;
		void* localConfig=NULL;
		if((rv=apr_proc_fork(proc,p))==APR_INCHILD){
			if(mpr->initFunc!=NULL){
				localConfig=(*mpr->initFunc)(p,mpr->userdata);
			}
			cm_internalMessagingLoop(p,mpr->nameSpace,mpr->logDir, mpr->messageBroker,localConfig, mpr->userdata,mpr->msgRecFunc);
			exit(1);
		}else{
			apr_pool_note_subprocess (p,proc,APR_KILL_AFTER_TIMEOUT);
			if ( !disableProcessRecovery ) {
			#if APR_HAS_OTHER_CHILD && MSG_PROC_RESTART
				apr_proc_other_child_register(proc, cm_messageProcessRestartCallback, proc, NULL, p);
			#endif
			}
		}
	}
#endif
	
	static void *APR_THREAD_FUNC cc_messagingThreadFunc(apr_thread_t*thread, void*params){
		void* localConfig=NULL;
		messaging_proc_rec* mpr=(messaging_proc_rec*)params;
		if(mpr->initFunc!=NULL){
			localConfig=(*mpr->initFunc)(mpr->p,mpr->userdata);
		}
		cm_internalMessagingLoop(mpr->p,mpr->nameSpace,mpr->logDir, mpr->messageBroker,localConfig, mpr->userdata,mpr->msgRecFunc);
		return NULL;
	}
	
	apr_proc_t* cm_initializeMessagingLoop(pool* p, char* nameSpace,char* logDir, cm_broker* messageBroker, void* userdata,cm_init_messaging_func initFunc, cm_message_recieved_func msgRecFunc,
			int disableProcessRecovery, int threaded){
		apr_status_t rv;
		root_mpr = cm_newMessagingProcRecObj(p, nameSpace, logDir, messageBroker, userdata, initFunc, msgRecFunc);
#if WIN32
	    apr_thread_t *t;
		rv = apr_thread_create(&t, NULL, cc_messagingThreadFunc, (void*)root_mpr, p);
		if(rv!=APR_SUCCESS) {
			printf("unable to created thread\r\n");fflush(stdout);
		}else{
			printf("thread successfully created\r\n");fflush(stdout);
		}
#else		
		if(threaded) {
		    apr_thread_t *t;
			rv = apr_thread_create(&t, NULL, cc_messagingThreadFunc, (void*)root_mpr, p);
			if(rv!=APR_SUCCESS) {
				printf("unable to created thread\r\n");fflush(stdout);
			}else{
				printf("thread successfully created\r\n");fflush(stdout);
			}
		}else{
			apr_proc_t* proc=(apr_proc_t*)apr_palloc(p,sizeof(apr_proc_t));			
			cm_startMessagingProcess(p, proc, root_mpr, disableProcessRecovery);
			return proc;
		}
#endif		
		return NULL;	
	}
	
