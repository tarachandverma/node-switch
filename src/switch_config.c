#ifndef __MODAUTH_PRODUCT_MAPPINGS__C_
#define __MODAUTH_PRODUCT_MAPPINGS__C_

#include "switch_config.h"
#include "switch_config_xml.h"
#include <common_utils.h>
#include "pcreposix.h"

#define SHEAP_ITEM_ID_SWITCH_MAPPINGS	"SWITCH_MAPPINGS"
	
	static switch_config* switchconf_newObj(shared_heap* sheap){
		switch_config* ret;
		
		ret=(switch_config*)shmdata_shpalloc(sheap,sizeof(switch_config));
		ret->modules=shmapr_hash_make(sheap);
		return ret;		
	}

	int switchconf_matchByStrings(pool* p, char* regex, char* value){
		int ret=0;
			
		//do regex match
		regex_t *preg=apr_palloc(p,sizeof(regex_t));

		ret=regcomp(preg,regex,0);
		if(ret!=0){
			return -555;	
		}
		
		ret=regexec(preg,value,0,NULL,0);		
		regfree(preg);

		return ret;
	}

	static int switchconf_isRegexValid(pool* p,char* regex){
		if(regex==NULL||switchconf_matchByStrings(p,regex,"")==-555){
			return 0;	
		}
		return 1;
	}
	
	static void switchconf_insertLong(pool*p, shared_heap* sheap, array_header* outArr, char* value) {
		long*place=(long*)shmapr_array_push(sheap, outArr);
		*place=atol(value);
	}
	
	static void switchconf_insertString(pool*p, shared_heap* sheap, array_header* outArr, char* value) {
		char**place=(char**)shmapr_array_push(sheap, outArr);
		*place=shmdata_32BitString_copy(sheap, value);		
	}
	
	static void switchconf_insertRegex(pool*p, shared_heap* sheap, array_header* outArr, char* value) {
		if(switchconf_isRegexValid(p,value)) {
			char**place=(char**)shmapr_array_push(sheap, outArr);
			*place=shmdata_32BitString_copy(sheap, value);
		}
	}
	
	//Build array from file.
	array_header* switchconf_buildFromFile(pool*p, shared_heap* sheap, 
			const char* filename,
			void (*insertFunc)(pool*p, shared_heap* sheap, array_header* outArr, char* value)) {
		apr_file_t* file=NULL;;
		apr_status_t status;
		char line[128], *comment=NULL;
		array_header* dstArr = NULL;
		char**place;
		
		if(filename==NULL)	return NULL;
		
		status = apr_file_open (&file, filename, APR_READ,APR_OS_DEFAULT,p);
		if(status!=APR_SUCCESS)	{
			printf("unable to open file\r\n");return NULL;
		}

		memset(line,'\0',sizeof(line));
		
		dstArr=shmapr_array_make(sheap,8,sizeof(char*));
		
		while((status=apr_file_gets (line,sizeof(line),file))==APR_SUCCESS){
			char* cpy = apr_pstrdup(p, line);
			char* value = apr_strtok(cpy, "#", &comment);
			value = astru_getTrimmedStr(p,value);
			if(insertFunc!=NULL&&value!=NULL) {
				(*insertFunc)(p, sheap, dstArr, value);
			}
		}
		apr_file_close ( file );
		
		return dstArr;
	}

	//Build map from file.
	shmapr_hash_t* switchconf_buildMapFromFile(pool*p, shared_heap* sheap, const char* filename) {
		apr_file_t* file=NULL;;
		apr_status_t status;
		char line[128], *comment=NULL;
		shmapr_hash_t* map = NULL;
		char **tokens;
		
		if(filename==NULL)	return NULL;
		
		status = apr_file_open (&file, filename, APR_READ,APR_OS_DEFAULT,p);
		if(status!=APR_SUCCESS)	{
			printf("unable to open file\r\n");return NULL;
		}

		memset(line,'\0',sizeof(line));
		
		map=shmapr_hash_make(sheap);
		
		while((status=apr_file_gets (line,sizeof(line),file))==APR_SUCCESS){
			char* cpy = apr_pstrdup(p, line);
			char* value = apr_strtok(cpy, "#", &comment);
			value = astru_getTrimmedStr(p,value);
			apr_tokenize_to_argv(value, &tokens, p);
			if(tokens[0]&&tokens[1]) {
				shmapr_hash_set(sheap,map,shmdata_32BitString_copy(sheap,tokens[0]),APR_HASH_KEY_STRING,shmdata_32BitString_copy(sheap,tokens[1]));
			}
		}
		apr_file_close ( file );
		
		return map;
	}
	
	static void switchconf_orderStringArray(array_header* prods){
		char **def=NULL, **defCur=NULL, *defTmp=NULL;
		int i=0, j=0;
		if(prods!=NULL&&prods->nelts>1){
			for(i=0;i<prods->nelts;i++){
				def=(char**)getElementRef(prods,i);
				for(j=i+1;j<prods->nelts;j++){
					defCur=(char**)getElementRef(prods,j);
					if(strcmp(*def,*defCur)>0){
//						printf("SWAPPING: %s,%s\r\n",*def,*defCur);
						defTmp=(*def);
						*def=*defCur;
						*defCur=defTmp;
						
					}
				}
			}
		}
	}
	
	static void switchconf_orderLongArray(array_header* prods){
		long *def=NULL, *defCur=NULL, defTmp=NULL;
		int i=0, j=0;
		if(prods!=NULL&&prods->nelts>1){
			for(i=0;i<prods->nelts;i++){
				def=(long*)getElementRef(prods,i);
				for(j=i+1;j<prods->nelts;j++){
					defCur=(long*)getElementRef(prods,j);
					if(*def>*defCur){
//						printf("SWAPPING: %d,%d\r\n",*def,*defCur);
						defTmp=(*def);
						*def=*defCur;
						*defCur=defTmp;
						
					}
				}
			}
		}
	}
	
	static switch_element* switchconf_newSwitchElementObj(pool*p, shared_heap* sheap, cc_globals* globals, switch_element_xml* hdrX){
		char* error = NULL, *listFile, *mapFile;
		array_header* stringArray, *longArray;
		switch_element* ret= (switch_element*)shmdata_shpcalloc(sheap, sizeof(switch_element));
		ret->name =  shmdata_32BitString_copy(sheap, hdrX->name);
		ret->type = hdrX->type;
		if(hdrX->value!=NULL) {
			switch (ret->type) {
			case switch_type_boolean:
				ret->value =  (void*)STRTOBOOL(hdrX->value);
				break;
			case switch_type_long:
				ret->value =  (void*)(atol(hdrX->value));
				break;		
			case switch_type_string:
				ret->value =  (void*)shmdata_32BitString_copy(sheap, hdrX->value);
				break;
			case switch_type_regex:
				ret->value =  (void*)shmdata_32BitString_copy(sheap, hdrX->value);
				break;			
			case switch_type_long_array:
				longArray = shmapr_parseLongArrayFromCsv(sheap,8,",",hdrX->value);
				switchconf_orderLongArray(longArray);
				ret->value =  (void*)longArray;
				break;
			case switch_type_string_array:
				stringArray = shmapr_parseStringArrayFromCsv(sheap,8,",",hdrX->value);
				switchconf_orderStringArray(stringArray);
				ret->value =  (void*)stringArray;
				break;
			case switch_type_regex_array:
				ret->value =  (void*)shmapr_parseStringArrayFromCsv(sheap,8,",",hdrX->value);
				break;				
			case switch_type_long_file:
				listFile=cc_getRemoteResourcePath(p,globals,hdrX->value,&error);
				if(listFile==NULL){
					listFile=cc_getLocalResourcePath(p, globals,hdrX->value,NULL);
				}
				longArray = switchconf_buildFromFile(p, sheap, listFile, switchconf_insertLong);
				switchconf_orderLongArray(longArray);
				ret->value = (void*)longArray;
				break;
			case switch_type_string_file:
				listFile=cc_getRemoteResourcePath(p,globals,hdrX->value,&error);
				if(listFile==NULL){
					listFile=cc_getLocalResourcePath(p, globals,hdrX->value,NULL);
				}
				stringArray = switchconf_buildFromFile(p, sheap, listFile, switchconf_insertString);
				switchconf_orderStringArray(stringArray);
				ret->value = (void*)stringArray;
				break;
			case switch_type_regex_file:
				listFile=cc_getRemoteResourcePath(p,globals,hdrX->value,&error);
				if(listFile==NULL){
					listFile=cc_getLocalResourcePath(p, globals,hdrX->value,NULL);
				}
				ret->value = (void*)switchconf_buildFromFile(p, sheap, listFile, switchconf_insertRegex);
				break;	
			}
		}
		return ret;
	}
	
	static switch_module* switchconf_newSwitchModuleObj(shared_heap* sheap, char* id){
		switch_module* ret=(switch_module*)shmdata_shpalloc(sheap,sizeof(switch_module));
			ret->id=shmdata_32BitString_copy(sheap,id);
			ret->params=shmapr_hash_make (sheap);
			return ret;
	}
	
	char* switchconf_build(pool* p,shared_heap* sheap, int isRefresh,cc_globals* globals,char* filepath){
		apr_status_t status;
		switch_mappings_xml* pmxml=NULL;
		switch_config*	ret=NULL;
		char* result=NULL;
		apr_hash_index_t *hi, *hiparam;
		switch_module_xml* moduleX=NULL;
		char* name=NULL,*value=NULL, *idname=NULL;
		switch_module* module=NULL;
		
		//stuff to handle config
		pool* subp=NULL;
		
		int x=0;
		
		//do on subpool
		if((status=apr_pool_create(&subp,p))!=APR_SUCCESS){
			return apr_pstrdup(p,"Could not create subpool");	
		}
		pmxml=switchconfxml_newObj(subp);
		result=switchconfxml_loadConfFile(subp,filepath,pmxml);
		if(result!=NULL){return result;}
		
		if(isRefresh==0){
			switchconfxml_printAll(subp,pmxml);
		}
		//end
		shmdata_OpenItemTag(sheap,SHEAP_ITEM_ID_SWITCH_MAPPINGS);
		ret=switchconf_newObj(sheap);

		for (hi = apr_hash_first(p, pmxml->modules); hi; hi = apr_hash_next(hi)) {
        	apr_hash_this(hi,(const void**)&idname, NULL, (void**)&moduleX);
        	
        	module=switchconf_newSwitchModuleObj(sheap,idname);
        	for (hiparam = apr_hash_first(p, moduleX->params); hiparam; hiparam = apr_hash_next(hiparam)) {
        		apr_hash_this(hiparam,(const void**)&name, NULL, (void**)&value);
        		switch_element* element = switchconf_newSwitchElementObj(subp, sheap, globals, (switch_element_xml*)(value));
        		shmapr_hash_set(sheap,module->params,name,APR_HASH_KEY_STRING,element);
        	}
        	shmapr_hash_set(sheap,ret->modules,idname,APR_HASH_KEY_STRING,module);
        	
		}
		
		shmdata_CloseItemTagWithInfo(sheap,"Switch Mappings");
		apr_pool_destroy(subp);
		
		switchconf_printSwitchConfigDetails(p, ret);
		fflush(stdout);

		return result;
	}
	
	void switchconf_printSwitchConfigDetails(pool* p,switch_config* conf){
		shmapr_hash_index_t *hi, *hiparam;
		switch_module* module=NULL;
		char* name=NULL,*value=NULL;
		
		logcore_printLog("\t<Switches Configuration - Shared heap>\n");
		
		logcore_printLog("t•\tSwitch Modules(%d)-\n",shmapr_hash_count (conf->modules));
		for (hi = shmapr_hash_first(p, conf->modules); hi; hi = shmapr_hash_next(hi)) {
        	shmapr_hash_this(hi,(const void**)&name, NULL, (void**)&module);
        	logcore_printLog("\t\t\t%s\n",name);
        	for (hiparam = shmapr_hash_first(p, module->params); hiparam; hiparam = shmapr_hash_next(hiparam)) {
        		shmapr_hash_this(hiparam,(const void**)&name, NULL, (void**)&value);
        		switch_element* element = (switch_element*)(value);
        		if(element!=NULL) {
        			switch (element->type) {
        			case switch_type_boolean:
        				logcore_printLog("\t\t\t\t%s:%s:%d\n",element->name,st_getSwitchTypeString(element->type),(int)element->value);
        				break;
        			case switch_type_long:
        				logcore_printLog("\t\t\t\t%s:%s:%d\n",element->name,st_getSwitchTypeString(element->type),(int)element->value);
        				break;	
        			case switch_type_string:
        				logcore_printLog("\t\t\t\t%s:%s:%s\n",element->name,st_getSwitchTypeString(element->type),(char*)element->value);
        				break;
        			case switch_type_regex:
        				logcore_printLog("\t\t\t\t%s:%s:%s\n",element->name,st_getSwitchTypeString(element->type),(char*)element->value);
        				break;		
        			case switch_type_long_array:
        			case switch_type_long_file:
        				logcore_printLog("\t\t\t\t%s:%s:%s\n",element->name,st_getSwitchTypeString(element->type),SAFESTR(astru_serializeCsvFromLongArray(p, (array_header*)element->value)));
        				break;
        			case switch_type_string_array:
        			case switch_type_string_file:
        				logcore_printLog("\t\t\t\t%s:%s:%s\n",element->name,st_getSwitchTypeString(element->type),SAFESTR(astru_serializeCsvFromStringArray(p, (array_header*)element->value)));
        				break;
        			case switch_type_regex_array:
        			case switch_type_regex_file:
        				logcore_printLog("\t\t\t\t%s:%s:%s\n",element->name,st_getSwitchTypeString(element->type),SAFESTR(astru_serializeCsvFromStringArray(p, (array_header*)element->value)));
        				break;
        			default: // unknown
        				logcore_printLog("\t\t\t\t%s:%s\n",element->name,st_getSwitchTypeString(element->type));
        				break;        				        				

        			}
        		}
        	}
		}
	}
	
	switch_config* switchconf_fetchFromSheap(shared_heap* sheap){
		return (switch_config*)shmdata_getItem(sheap,SHEAP_ITEM_ID_SWITCH_MAPPINGS);
	}
#endif


