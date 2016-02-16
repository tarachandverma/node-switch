#ifndef __WSJACL_CONFIG_ERROR__H_
#define __WSJACL_CONFIG_ERROR__H_

#include <sys/types.h>
#include <apache_mappings.h>

	typedef struct ce_error_list{
		apr_array_header_t* data;
	}ce_error_list;
	
	ce_error_list* ce_newErrorListObj(pool* p);
	void ce_addError(pool* p,ce_error_list* elist, char* msg);
	void ce_addErrorWithType(pool* p,ce_error_list* elist, char* mtype,char* msg);
	void ce_printList(pool* p,ce_error_list* elist);
	int ce_hasErrors(ce_error_list* elist);
#endif

