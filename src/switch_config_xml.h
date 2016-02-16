#ifndef __MODAUTH_SWITCH_CONFIG_XML__H_
#define __MODAUTH_SWITCH_CONFIG_XML__H_
#include <apache_mappings.h>
#include "switch_types.h"

	typedef struct switch_element_xml{
		char* name;
		switch_type type;
		char* value;
	}switch_element_xml;

	typedef struct switch_module_xml{
		char* id;
		apr_hash_t* params;
	}switch_module_xml;
	
	typedef struct switch_mappings_xml{
		apr_hash_t* modules;		
	}switch_mappings_xml;
	
	switch_mappings_xml* switchconfxml_newObj(pool* p);
	char* switchconfxml_loadConfFile(pool* p, char* file, switch_mappings_xml* conf);
	void switchconfxml_printAll(pool* p,switch_mappings_xml* conf);
#endif
