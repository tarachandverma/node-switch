#ifndef __WSJACL_CONFIG_MESSAGING_PARSING__H_
#define __WSJACL_CONFIG_MESSAGING_PARSING__H_

#include <sys/types.h>
#include <apache_mappings.h>
	typedef struct cm_wire_header{
		char* nodeName;
		char* nameSpace;
		char* ipList;
		char* componentId;
		char* version;
	}cm_wire_header;
	
	typedef struct cm_wire_message{
		char* type;
		char* batchId;
		cm_wire_header* header;
		apr_hash_t* params;
	}cm_wire_message;
	
	cm_wire_header* cm_newWireHeader(pool* p, char* nameSpace);
	cm_wire_message* cm_newWireMessage(pool* p,cm_wire_header* header,char* batchId);
	cm_wire_message* cm_newWireMessageType(pool* p,const char* type,cm_wire_header* header,char* batchId);
	cm_wire_message* cm_parseMessage(apr_pool_t* p,char* src);
	char* cm_serializeMessage(apr_pool_t* p, cm_wire_message* msg);
#endif

