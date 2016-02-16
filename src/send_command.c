#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "config_core2.h"
#include "config_error2.h"
#include "common_utils.h"

int die(int exitCode, const char *message, apr_status_t reason) {
    char msgbuf[80];
	apr_strerror(reason, msgbuf, sizeof(msgbuf));
	fprintf(stderr, "%s: %s (%d)\n", message, msgbuf, reason);
	exit(exitCode);
	return reason;
}

static void terminate()
{
   apr_terminate();
}

static char* sc_sendMessage(pool* p, config_core* config, char* action, char* namespce){
	apr_status_t rc;
	char* error=NULL;
	cm_wire_message* msg=NULL;
	cm_connection* cmConn=cm_newConnectionObj(p,config->globals->nameSpace);
	
	//disconnect
	error=cm_connect(p,cmConn,config->messageBroker,2);
	if(error==NULL){
		if(strcmp(action,"Refresh")==0){
			rc=cm_sendRefresh(p,cmConn,namespce);
		}else{
			return apr_pstrcat(p,"Command not recognized:",SAFESTR(action),NULL);
		}
		if(rc!=APR_SUCCESS){
			error=apr_strerror(rc,apr_pcalloc(p,128),128);
		}
		cm_disconnect(p,cmConn);
	}
	return error;
}

typedef struct sc_args{
	char* action;
	char* namespace;
	char* configFile;
}sc_args;

sc_args* sc_parseArguments(pool*p, int argc, char*argv[]){
	sc_args* args = (sc_args*)apr_pcalloc(p, sizeof(sc_args));
	args->action=apr_pstrdup(p, "Refresh");
	
	int i;
	for(i=2; i < argc; i+=2){
		const char* name = argv[i-1];
		const char* value = argv[i];
		if(name[0]=='-'){
			switch(name[1]){
			case 'a':
				args->action = apr_pstrdup(p, value);
				break;
			case 'n':
				args->namespace = apr_pstrdup(p, value);
				break;
			case 'c':
				args->configFile = apr_pstrdup(p, value);
				break;
			}
		}
	}
	//Validate and print usage details if options are not set correctly.
	if(args->configFile==NULL || args->namespace==NULL) {
		printf("Usage: ./sc -n <namespace> -c <config-file> [-a <action, default:Refresh>] \r\n");
		return NULL;
	}
	return args;
}

int main(int argc, char *argv[])
{
	
	pool* p=NULL,*subp=NULL;
	apr_status_t status;
	char* errorJms = NULL;
	
	setbuf(stdout, NULL);	
	status = apr_initialize();
	status==APR_SUCCESS || die(-2, "Could not initialize", status);
	atexit(terminate);	
   
	status = apr_pool_create(&p, NULL);
	status==APR_SUCCESS || die(-2, "Could not allocate pool", status);
	
	sc_args* args = sc_parseArguments(p, argc, argv);
	
	if(args==NULL){
		return 0;
	}
	
	ce_error_list* errorList=ce_newErrorListObj(p);
	config_core* configCore=cc_newConfigCoreObj(p);
	char* error=cc_loadConfigCoreFile(p, args->configFile, configCore);
	ce_addErrorWithType(p,errorList,"Config Core Load File",error);
	if(error==NULL){
		cc_printConfigCoreDetails(p,configCore);
	}
	
	// send refresh message
	errorJms=sc_sendMessage(p,configCore,args->action,args->namespace);
	if(errorJms!=NULL){
		printf("Action: %s... Unable to send due to Jms Problem: %s\n",args->action,errorJms);
	}else{
		printf("Action: %s... Sent\n", args->action);
	}
	
	apr_pool_destroy(p);
   return 0;
}
