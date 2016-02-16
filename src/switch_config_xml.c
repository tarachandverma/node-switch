#include <apache_mappings.h>
#include <xml_core.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common_utils.h>
#include "switch_config_xml.h"


	typedef struct pmap_tmp{
		switch_mappings_xml* conf;
		void* tmp;
		char* str;
		void* userdata,*userdata2;
	}pmap_tmp;
	
	static switch_module_xml* accx_newAuthHandlerObj(pool* p, char* id){
		switch_module_xml* ret=(switch_module_xml*)apr_palloc(p,sizeof(switch_module_xml));
			ret->id=apr_pstrdup(p,id);
			ret->params=apr_hash_make (p);
			return ret;
	}
	
	switch_mappings_xml* switchconfxml_newObj(pool* p){
		switch_mappings_xml* ret=apr_palloc(p,sizeof(switch_mappings_xml));
		ret->modules=apr_hash_make(p);
		return ret;
	}
	
	static int switchconfxml_setACCHandlerAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		switch_module_xml* handler=NULL;
		int i;
		pmap_tmp* bundle=(pmap_tmp*)userdata;
		bundle->userdata=NULL;
		for (i = 0; attributes[i]; i += 2) {
			if(strcmp(attributes[i],"id")==0){
				handler=accx_newAuthHandlerObj(p,(char*)attributes[i+1]);
				bundle->userdata=(void*)handler;
			} 			
  		}
		return 1;
	}
	
	static switch_element_xml* switchconfxml_newSwitchElementObj(pool* p){
		switch_element_xml* ret=(switch_element_xml*)apr_palloc(p,sizeof(switch_element_xml));
		ret->name=NULL;
		ret->type=switch_type_unknown;
		ret->value=NULL;
		return ret;
	}

	static switch_type switchconfxml_getSwitchType(const char* type) {
		switch_type stype = switch_type_boolean;
		
		if(type!=NULL) {
			if (!strcasecmp(type, "boolean"))
				stype = switch_type_boolean;
		    else if (!strcasecmp(type, "long"))
		    	stype = switch_type_long;
		    else if (!strcasecmp(type, "string"))
		    	stype = switch_type_string;
		    else if (!strcasecmp(type, "regex"))
		    	stype = switch_type_regex;			
		    else if (!strcasecmp(type, "longArray"))
		    	stype = switch_type_long_array;
		    else if (!strcasecmp(type, "stringArray"))
		    	stype = switch_type_string_array;
		    else if (!strcasecmp(type, "regexArray"))
		    	stype = switch_type_regex_array;			
		    else if (!strcasecmp(type, "longFile"))
		    	stype = switch_type_long_file;
		    else if (!strcasecmp(type, "stringFile"))
		    	stype = switch_type_string_file;
		    else if (!strcasecmp(type, "regexFile"))
		    	stype = switch_type_regex_file;			
		}
		return stype;
	}
	
	static int switchconfxml_setACCHandlerParamAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		int i;
		pmap_tmp* bundle=(pmap_tmp*)userdata;
		bundle->userdata2=NULL;
		switch_element_xml* element = switchconfxml_newSwitchElementObj(p);
		for (i = 0; attributes[i]; i += 2) {
			if(strcmp(attributes[i],"name")==0){
				element->name=(void*)apr_pstrdup(p,attributes[i + 1]);
			} else if(strcmp(attributes[i],"type")==0){
				element->type = switchconfxml_getSwitchType((char*)attributes[i + 1]);
			} 			
  		}
		bundle->userdata2 = (void*)element;
		return 1;
	}
	static int switchconfxml_setACCHandlerParamBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		pmap_tmp* bundle=(pmap_tmp*)userdata;
		switch_module_xml* handler=NULL;
		switch_element_xml* element = (switch_element_xml*)bundle->userdata2;
		
		if(bundle!=NULL&&bundle->userdata!=NULL&&element!=NULL){
			handler=(switch_module_xml*)bundle->userdata;
			element->value = apr_pstrdup(p,body);
			apr_hash_set(handler->params,(char*)element->name,APR_HASH_KEY_STRING,element);
		}
		bundle->userdata2=NULL;
		return 1;
	}
	static int switchconfxml_setACCHandlerEnd(pool* p,char* xPath,int type,void* userdata){
		pmap_tmp* bundle=(pmap_tmp*)userdata;
		switch_module_xml* handler=NULL;
		
		if(bundle!=NULL&&bundle->userdata!=NULL){
			handler=(switch_module_xml*)bundle->userdata;
			apr_hash_set (bundle->conf->modules,handler->id,APR_HASH_KEY_STRING,handler);
		}
		
		bundle->userdata=NULL;
		bundle->userdata2=NULL;
		return 1;
	}
	
	char* switchconfxml_loadConfFile(pool* p, char* file, switch_mappings_xml* conf){
		XmlCore* xCore;
		pmap_tmp tmp;
		char* result=NULL;
		
		tmp.conf=conf;
		tmp.tmp=NULL;
		tmp.str=NULL;
		
		xCore=xmlcore_getXmlCore(p);
		
		xmlcore_addXPathHandler(xCore,"/switch-config/modules/module",0,switchconfxml_setACCHandlerAttributes,NULL,switchconfxml_setACCHandlerEnd, &tmp);
		xmlcore_addXPathHandler(xCore,"/switch-config/modules/module/switch",0,switchconfxml_setACCHandlerParamAttributes,switchconfxml_setACCHandlerParamBody,NULL,&tmp);

		result=xmlcore_beginParsingTextResponse(xCore,file);
		return result;
	}
		
	void switchconfxml_printAll(pool* p,switch_mappings_xml* conf){
		int x=0;
		apr_hash_index_t *hi, *hiparam;
		switch_module_xml* handler=NULL;
		char* name=NULL,*value=NULL;
		
		if(conf==NULL) return;
		
		printf("<Switches Configuration>\r\n");
				
		printf("\t\t•\tModules(%d)-\n",apr_hash_count (conf->modules));
		for (hi = apr_hash_first(p, conf->modules); hi; hi = apr_hash_next(hi)) {
        	apr_hash_this(hi,(const void**)&name, NULL, (void**)&handler);
        	printf("\t\t\t\t%s\n",name);
        	for (hiparam = apr_hash_first(p, handler->params); hiparam; hiparam = apr_hash_next(hiparam)) {
        		apr_hash_this(hiparam,(const void**)&name, NULL, (void**)&value);
        		switch_element_xml* element = (switch_element_xml*)(value);
        		if(element!=NULL) {
        			printf("\t\t\t\t\t%s:%s:%s\n",element->name,st_getSwitchTypeString(element->type),element->value);
        		}
        	}
		}
	}
	


