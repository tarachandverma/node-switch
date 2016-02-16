#ifndef __WSJACL_CONFIG_MESSAGING__H_
#define __WSJACL_CONFIG_MESSAGING__H_

#include <sys/types.h>
#include <apache_mappings.h>
#include "config_messaging_parsing2.h"
#include <stomp-utils/stomp_utils.h>
#include "apr_thread_proc.h"

	typedef struct cm_broker{
		char *host,*username, *password;
		int port;
		long reconnectWaitMs;
		long waitTimeoutSeconds;
	}cm_broker;
	
	typedef struct cm_connection{
		cm_wire_header* wireHeader;
		stomp_connection* stomp;
	}cm_connection;
	
	typedef char* (*cm_message_recieved_func) (pool*,cm_connection*,cm_wire_message*,void*, void*, int);
	typedef void* (*cm_init_messaging_func)(pool*,void*);
	
	cm_broker* cm_newMessageBrokerObj(pool* p);
	char* cm_testMessageBroker(pool* p, char* nameSpace, cm_broker* messageBroker);
	apr_status_t cm_sendMessage(pool* p, cm_connection* conn, cm_wire_message* msg);
	cm_wire_message* cm_getNextWireMessage(pool* p, cm_connection* cmConn);
	apr_proc_t* cm_initializeMessagingLoop(pool* p, char* nameSpace,char* homeDir, cm_broker* messageBroker,void* userdata,cm_init_messaging_func initFunc, cm_message_recieved_func msgRecFunc,int disableProcessRecovery,int threaded);
	cm_connection* cm_newConnectionObj(pool* p, char* nameSpace);
	char* cm_connect(pool* p,cm_connection* cmConn, cm_broker* messageBroker, apr_int32_t timeoutSeconds);
	char* cm_disconnect(pool* p,cm_connection* cmConn);
	apr_status_t cm_sendRefresh(pool* p, cm_connection* cmConn,char* ns);
	apr_status_t cm_sendPing(pool* p, cm_connection* cmConn,char* ns);
	apr_status_t cm_sendPong(pool* p, cm_connection* cmConn, char* batchId);
	apr_status_t cm_sendRefreshComplete(pool* p, cm_connection* cmConn,char* batchId);
	apr_status_t cm_sendRefreshFailed(pool* p, cm_connection* cmConn,char* batchId,char* errors);
	apr_status_t cm_sendAlive(pool* p, cm_connection* cmConn,char* ns);
	int cm_isAliveMessage(cm_wire_message* msg);
	int cm_isPingMessage(cm_wire_message* msg);
	int cm_isRefreshMessage(cm_wire_message* msg);
	int cm_isRefreshCompleteMessage(cm_wire_message* msg);
	int cm_isRefreshFailedMessage(cm_wire_message* msg);
	char* cm_newMessageBatchId(pool* p);
#endif

