#include "libswitch_conf.h"
#include <apache_typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config_error2.h"
#include "common_utils.h"

#define MODULE_NAME_SWITCH_CONFIG "switchConfig"

int libswitch_validBootStatus(boot_status* bstat){
	return	bstat->loaded_params==REQUIRED_PARAMS;
}

void bootstatus_zero(pool* p, boot_status* bstat){
	bstat->loaded_params=0;	
	bstat->uptime=time(NULL);
}

void bootstatus_increment_param(boot_status* bstat){
	bstat->loaded_params++;
} 

void bootstatus_cpy(boot_status* src, boot_status* dst){
	dst->loaded_params=src->loaded_params;
	dst->uptime=src->uptime;
}

static int libswitch_is_config_loaded(Config* config){
	int items=0;

	if(config->config_core_file!=NULL){
				items++;	
		}
	
	return items==6;
}


static void libswitch_config_zero(pool *p, Config* config){
	//init http stuff here because need a refactor to do it cleaner

	bootstatus_zero(p,&(config->boot_info));
	config->systemInfo=NULL;
	
	config->home_dir=NULL;
	config->config_core_file=NULL;
	config->logfile_name=NULL;
	config->configCore=NULL;
	config->switchConfig=NULL;
}

static Config* libswitch_newConfigObj(pool* p){
	Config* ret=(Config*)apr_palloc(p,sizeof(Config));
	libswitch_config_zero(p,ret);
	return ret;
}

char* libswitch_setSystemInfo(pool* p, Config* config, char* arg){
	config->systemInfo = apr_pstrdup(p, arg);
	ACLCONF_OUT(config,VERSION_ID);
	ACLCONF_OUT1(config,"SystemInfo:%s\n\n",config->systemInfo);
return NULL;
}

char* libswitch_postRefreshBind(apr_pool_t* p,Config* config){
	if(config->configCore==NULL){
		return apr_pstrdup(p,"Config Core is NULL");
	}
	
	config->switchConfig=(switch_config*)cc_getModuleConfigByName(config->configCore,MODULE_NAME_SWITCH_CONFIG);		
	if(config->switchConfig==NULL){
		return apr_pstrcat(p,"! Missing Required Service Name:",MODULE_NAME_SWITCH_CONFIG,NULL);
	}
	
	return NULL;
}

ls_status_t libswitch_postConfig(apr_pool_t* p, apr_pool_t* plog, apr_pool_t* ptemp, Config* config){
	config_core* ccore=NULL;
	ce_error_list* errorList=NULL;
	char* error=NULL;
	char cbuf[APR_CTIME_LEN + 1];
	
	// normalize paths
	config->config_core_file = apr_pstrcat(p, config->home_dir, "/", config->config_core_file, NULL);
	config->logfile_name = apr_pstrcat(p, config->home_dir, "/", config->logfile_name, NULL);

	logcore_openLogFile(p,config->logfile_name);
	logcore_truncateLogFile();
	apr_time_t t1 = apr_time_now();
	apr_ctime(cbuf, t1);
	logcore_printLog("\n¬∞\t Initializing %s [%s][%s]\n",VERSION_ID,config->systemInfo,cbuf);

	fflush(stdout);
	http_init();
	
	fflush(stdout);
	//printf("‚àö Success using the following configuration:\r\n");
	fflush(stdout);
	//init Error Obj
	errorList=ce_newErrorListObj(p);
	
	//load crypto core
	logcore_printLog("\r\n\r\n");
	ccore=cc_newConfigCoreObj(p);
	ccore->globals->homeDir=apr_pstrdup(p,config->home_dir);
	ccore->globals->logsDir=apr_pstrcat(p,config->home_dir,"/logs",NULL);
	ccore->refreshLogFile=apr_pstrdup(p,config->logfile_name);
	error=cc_loadConfigCoreFile(p, config->config_core_file, ccore);
	ce_addErrorWithType(p,errorList,"Config Core Load File",error);
	if(error==NULL){
		cc_printConfigCoreDetails(p,ccore);
		error=cc_initializeConfigCore(p,ccore);
		ce_addErrorWithType(p,errorList,"Config Core Init",error);
		config->configCore=ccore;
		logcore_printLog("‚àö Config Core Initialized\n");
		
		logcore_printLog("¬∞ Binding Config Core Services\n");
		error=libswitch_postRefreshBind(p,config);
		ce_addErrorWithType(p,errorList,"Config Core Bind",error);
	}

	if(!ce_hasErrors(errorList)){
		logcore_printLog("\r\n>> %s [%s] - Libswitch initialized <<\r\n",VERSION_ID,config->systemInfo);
	}else{
		logcore_printLog("\r\n>> %s [%s] - Libswitch initialized WITH ERRORS<<\r\n",VERSION_ID,config->systemInfo);
		ce_printList(p,errorList);
		return LS_FAILURE;
	}
	return LS_SUCCESS;
}

char* libswitch_setConfigCoreFile(pool* p, Config* config,char* arg){
	config->config_core_file=apr_pstrdup(p,arg);
	bootstatus_increment_param(&(config->boot_info));
	return NULL;
}

char* libswitch_setLogFile(pool* p, Config* config,char* arg){
	config->logfile_name=apr_pstrdup(p,arg);
	bootstatus_increment_param(&(config->boot_info));
	return NULL;
}

char* libswitch_setHomeDir(pool* p, Config* config,char* arg){
	config->home_dir=apr_pstrdup(p,arg);
	bootstatus_increment_param(&(config->boot_info));
	return NULL;
}

typedef char* (*command_handler_func) (pool*, Config*, char* arg);

typedef struct command_handler_rec{
	char* id;
	command_handler_func handlerFunc;
	char* description;
}command_handler_rec;

static const command_handler_rec wsjacl_cmds[] =
{
	{"LIBSWITCH_SystemInfo", libswitch_setSystemInfo,  "String for system info - used in status messages"},
	{"LIBSWITCH_ConfigCoreFile", libswitch_setConfigCoreFile, "Set Config Core filename"},
	{"LIBSWITCH_LogFile", libswitch_setLogFile, "Set Config Core logfile"},
	{"LIBSWITCH_HomeDir", libswitch_setHomeDir, "Set Home directory"}
};

static void libswitch_runCommandHandler(apr_pool_t*p, Config*config, const char* id, char* arg){
	int i;
	int len=sizeof(wsjacl_cmds)/sizeof(command_handler_rec);
	for(i=0; i<len; i++){
		if(wsjacl_cmds[i].id!=NULL&&strcmp(wsjacl_cmds[i].id,id)==0){
			(wsjacl_cmds[i].handlerFunc)(p, config, arg);
		}
	}
}

static char* libswitch_parseConfigFile(apr_pool_t* p, const char* conf, Config* config) {
	apr_file_t* file=NULL;;
	apr_status_t status;
	char line[128];
	char* comment;
	char* command, *arg;
	char **tokens;
	int i, len;

	status = apr_file_open (&file, conf, APR_READ,APR_OS_DEFAULT,p);
	if(status!=APR_SUCCESS)	{
		return apr_pstrdup(p, "Unable to open config file");
	}

	memset(line,'\0',sizeof(line));

	while((status=apr_file_gets (line,sizeof(line),file))==APR_SUCCESS){
		char* cpy = astru_getTrimmedStr(p,line);
		if(cpy!=NULL&&*cpy!='#') {
			char* value = apr_strtok(cpy, "#", &comment);
			apr_tokenize_to_argv(value, &tokens, p);
			libswitch_runCommandHandler(p, config, tokens[0], tokens[1]);
		}
	}
	apr_file_close ( file );
	return NULL;
}

Config* gConfig = NULL;
apr_pool_t* mainPool = NULL;
apr_pool_t* workerPool = NULL;

static void libswitch_terminate() {
   apr_terminate();
}

static int libswitch_die(int exitCode, const char *message, apr_status_t reason) {
    char msgbuf[80];
	apr_strerror(reason, msgbuf, sizeof(msgbuf));
	fprintf(stderr, "%s: %s (%d)\n", message, msgbuf, reason);
	exit(exitCode);
	return reason;
}

static ls_status_t libswitch_initializeLibSwitchINT(const char* configFile) {
	apr_status_t aprstatus;
	
	// Initialize library
	aprstatus = apr_initialize();
	if(aprstatus!=APR_SUCCESS) { libswitch_die(-2, "Could not initialize", aprstatus); }
	atexit(libswitch_terminate);
	
	// create mainPool
	aprstatus = apr_pool_create(&mainPool, NULL);
	if(aprstatus!=APR_SUCCESS) { libswitch_die(-2, "Could not initialize main pool", aprstatus); }
	
    // create a request pool.
	aprstatus = apr_pool_create(&workerPool, mainPool);
	if(aprstatus!=APR_SUCCESS) { libswitch_die(-2, "Could not initialize worker pool", aprstatus); }
	
	gConfig = libswitch_newConfigObj(mainPool);
	char* error = libswitch_parseConfigFile(mainPool, configFile, gConfig);
	
	if(error!=NULL) return LS_FAILURE;
	
	//normalize relative to home directory
	if(gConfig->home_dir==NULL) return LS_FAILURE;

	return libswitch_postConfig(mainPool, mainPool, mainPool, gConfig);
}

ls_status_t libswitch_initializeLibSwitch(const char* configFile) {
	if(gConfig!=NULL) { 
		return LS_SUCCESS; // already initialized.
	}
	return libswitch_initializeLibSwitchINT(configFile);
}

// binary search for long values
static int libswitch_bsearchLong(array_header* arr, int low, int high, long x) {
	if ( low > high ) return -1;
	printf("libswitch_bsearchLong: low=%d high=%d\r\n", low, high);fflush(stdout);
	
	int mid = ( low + high ) / 2;
	long midElement = (long)getElement(arr, mid);
	if(midElement == x) 
		return mid;
	else if (midElement < x) 
		return libswitch_bsearchLong(arr, mid+1, high, x);
	else
		return libswitch_bsearchLong(arr, low, mid-1, x);
}

static int libswitch_isLongArrayMatched(array_header* arr, long value) {
	if(arr==NULL) return FALSE;
	return (libswitch_bsearchLong(arr, 0, arr->nelts-1, value)<0)  ? FALSE : TRUE;
}

// binary search for strings
static int libswitch_bsearchString(array_header* arr, int low, int high, const char* x) {
	if ( low > high ) return -1;
	
	int mid = ( low + high ) / 2;
	char* midElement = (char*)getElement(arr, mid);
	int result = strcmp(midElement, x);
	if(result==0) 
		return mid;
	else if (result<0) 
		return libswitch_bsearchString(arr, mid+1, high, x);
	else
		return libswitch_bsearchString(arr, low, mid-1, x);
}

static int libswitch_isStringArrayMatched(array_header* arr, const char* value) {
	if(arr==NULL) return FALSE;
	
	return (libswitch_bsearchString(arr, 0, arr->nelts-1, value)<0) ? FALSE : TRUE;
}

static int libswitch_isRegexArrayMatched(pool*p, array_header* arr, const char* value) {
	int i;
	char* e1;
	
	if(arr==NULL) return FALSE;
	
	for(i=0; i<arr->nelts; i++){
		e1=(char*)getElement(arr, i);
		if(switchconf_matchByStrings(p, e1,value)==0){
			return TRUE;
		}
	}
	return FALSE;
}

int libswitch_isSwitchEnabled(const char* moduleId, const char* switchName, const char* switchValue) {
	pool* localp;
	int switchBoolean = 0;
	apr_status_t pstat = apr_pool_create(&localp, workerPool);
	if ( (pstat != APR_SUCCESS) || (localp == NULL) ) {
		// throw some error
		return 0;
	}
	if(cc_syncSelf(localp, gConfig->configCore)>0){
		libswitch_postRefreshBind(localp, gConfig);
	}
	
	if ( gConfig->switchConfig == NULL ) {
		// throw some error
		apr_pool_destroy(localp);
		return 0;
	}
	
	switch_module* module = (switch_module*)shmapr_hash_get(gConfig->switchConfig->modules, moduleId, APR_HASH_KEY_STRING);

	if(module==NULL) return 0;

	switch_element* element = (switch_element*)shmapr_hash_get(module->params, switchName, APR_HASH_KEY_STRING);
	
	if(element==NULL||element->value==NULL||switchValue==NULL)  return 0;
	
	if(element!=NULL) {
		switch (element->type) {
		case switch_type_boolean:
			switchBoolean = (STRTOBOOL(switchValue) == (int)element->value);
			break;
		case switch_type_long:
			switchBoolean = (atol(switchValue) == (long)element->value);
			break;	
		case switch_type_string:
			switchBoolean = (strcmp((char*)element->value, switchValue)==0);
			break;
		case switch_type_regex:
			switchBoolean = (switchconf_matchByStrings(localp, (char*)element->value, switchValue)==0);
			break;
		case switch_type_long_array:
		case switch_type_long_file:
			switchBoolean = libswitch_isLongArrayMatched((array_header*)element->value, atol(switchValue));
			break;			
		case switch_type_string_array:
		case switch_type_string_file:
			switchBoolean = libswitch_isStringArrayMatched((array_header*)element->value, switchValue);
			break;
		case switch_type_regex_array:
		case switch_type_regex_file:
			switchBoolean = libswitch_isRegexArrayMatched(localp, (array_header*)element->value, switchValue);
			break;				
		}        			
	}
	
	apr_pool_destroy(localp);
	
	return switchBoolean;
}

