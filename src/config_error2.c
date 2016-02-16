#ifndef __WSJACL_CONFIG_BINDINGS__C_
#define __WSJACL_CONFIG_BINDINGS__C_

#include <apache_mappings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include "config_error2.h"
#include <common_utils.h>

	ce_error_list* ce_newErrorListObj(pool* p){
		ce_error_list* ret=(ce_error_list*)apr_pcalloc(p,sizeof(ce_error_list));
		ret->data=apr_array_make(p,4,sizeof(char*));\
		return ret;
	}
	
	void ce_addErrorWithType(pool* p, ce_error_list* elist, char* mtype, char* msg){
		char** place=NULL;
		char* item=NULL;
		
		if(elist==NULL||msg==NULL) return;

		if(mtype!=NULL){
			item=apr_pstrcat(p,"[",mtype,"] ",msg,NULL);	
		}else{
			item=apr_pstrdup(p,msg);
		}
		place=(char**)apr_array_push(elist->data);
		*place=item;
	}
	
	void ce_addError(pool* p,ce_error_list* elist, char* msg){
		ce_addErrorWithType(p,elist,NULL,msg);
	}
	void ce_printList(pool* p,ce_error_list* elist){
		int i=0;
		char* item=NULL;
		apr_array_header_t* arry=NULL;
		if(elist==NULL) return;
		
		arry=elist->data;
		for(i=0;i<arry->nelts;i++){
			item=getElement(arry,i);
			printf("• %s\n",item);
		}
	}
	int ce_hasErrors(ce_error_list* elist){
		return elist!=NULL&&elist->data->nelts>0;
	}
#endif
