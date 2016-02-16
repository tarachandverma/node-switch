#ifndef __WSJACL_CONFIG_BINDINGS__H_
#define __WSJACL_CONFIG_BINDINGS__H_

#include <sys/types.h>
#include <apache_mappings.h>

	typedef struct cc_service_descriptor{
		char* id;
		char* name;
		char* uri;
		char* userColonPass;
		long timeoutSeconds;
		apr_hash_t * params;
	}cc_service_descriptor;
	
	typedef struct cc_globals{
		char* nameSpace;
		char* homeDir;
		char* logsDir;
		cc_service_descriptor* resourceService;
	}cc_globals;
	
	cc_service_descriptor* cc_newServiceDescriptorObj(pool* p);
	cc_globals* cc_newGlobalsObj(pool* p);
	char* cc_initGlobals(pool* p,cc_globals* globals);
	char* cc_getRemoteResourcePath(pool* p, cc_globals* globals,char* resource,char**details);
	char* cc_getLocalResourcePath(pool* p, cc_globals* globals,char* resource,char**details);
#endif

