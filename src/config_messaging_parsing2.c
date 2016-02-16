#ifndef __WSJACL_CONFIG_MESSAGING_PARSING__C_
#define __WSJACL_CONFIG_MESSAGING_PARSING__C_

#include <apache_mappings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <xml_core.h>
#include "config_messaging_parsing2.h"
#include <common_utils.h>
#include "libswitch_version.h"
#include <limits.h>
	
	typedef struct cm_parse_bundle{
		char* tmp;
		cm_wire_message* msg;
	}cm_parse_bundle;
		
		
	/**
	 * Begin Wire Message Code
	 */
	static cm_wire_header* cm_newWireHeaderBlank(pool* p){	
		cm_wire_header* ret=(cm_wire_header*)apr_palloc(p,sizeof(cm_wire_header));
		ret->nodeName=NULL;
		ret->nameSpace=NULL;
		ret->ipList=NULL;
		ret->componentId=NULL;
		ret->version=NULL;
		return ret;
	} 
	cm_wire_header* cm_newWireHeader(pool* p,char* nameSpace){
		int i;
		char buf[128];
		
		//for second ip get test		
		cm_wire_header* ret=cm_newWireHeaderBlank(p);
		if(nameSpace!=NULL){
			if(gethostname(buf,128)==0){
				ret->nodeName=apr_pstrdup(p,buf);
			}
			ret->nameSpace=(nameSpace!=NULL?apr_pstrdup(p,nameSpace):NULL);
			if(ret->nodeName!=NULL){
				//get ips
				ret->ipList=comu_getNodeDetails(p,80);
			}else{
				ret->ipList=NULL;
			}
			ret->componentId=apr_pstrdup(p,MODULE_COMPONENT_ID);
			ret->version=apr_pstrdup(p,MODULE_VERSION_ID);
		}

		return ret; 
	}
	cm_wire_message* cm_newWireMessage(pool* p,cm_wire_header* header,char* batchId){
		char buf[64];
		cm_wire_message* ret=(cm_wire_message*)apr_palloc(p,sizeof(cm_wire_message));
		ret->type=NULL;
		ret->batchId=NULL;
		ret->header=header;
		ret->params=apr_hash_make(p);
		
		if(batchId!=NULL){
			ret->batchId=apr_pstrdup(p,batchId);
		}
				
		
		sprintf(buf,"%d",getpid());
		apr_hash_set(ret->params,"pid",APR_HASH_KEY_STRING,apr_pstrdup(p,buf));
		
		return ret;
	}
	cm_wire_message* cm_newWireMessageType(pool* p,const char* type,cm_wire_header* header,char* batchId){
		cm_wire_message* ret=cm_newWireMessage(p,header,batchId);
		ret->type=apr_pstrdup(p,type);
		return ret;
	}
	
	static int cm_setMessageAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		cm_parse_bundle* bundle=(cm_parse_bundle*)userdata;
		int i;
		
		for (i = 0; attributes[i]; i += 2) {
			if(strcmp(attributes[i],"type")==0){
				bundle->msg->type=apr_pstrdup(p,attributes[i + 1]);
			}else if(strcmp(attributes[i],"node")==0){
				if(bundle->msg->header==NULL){
					bundle->msg->header=cm_newWireHeaderBlank(p);
				}
				bundle->msg->header->nodeName=apr_pstrdup(p,attributes[i + 1]);
			}else if(strcmp(attributes[i],"namespace")==0){
				if(bundle->msg->header==NULL){
					bundle->msg->header=cm_newWireHeaderBlank(p);
				}
				bundle->msg->header->nameSpace=apr_pstrdup(p,attributes[i + 1]);
			}else if(strcmp(attributes[i],"ips")==0){
				if(bundle->msg->header==NULL){
					bundle->msg->header=cm_newWireHeaderBlank(p);
				}
				bundle->msg->header->ipList=apr_pstrdup(p,attributes[i + 1]);
			}else if(strcmp(attributes[i],"com")==0){
				if(bundle->msg->header==NULL){
					bundle->msg->header=cm_newWireHeaderBlank(p);
				}
				bundle->msg->header->componentId=apr_pstrdup(p,attributes[i + 1]);
			}else if(strcmp(attributes[i],"ver")==0){
				if(bundle->msg->header==NULL){
					bundle->msg->header=cm_newWireHeaderBlank(p);
				}
				bundle->msg->header->version=apr_pstrdup(p,attributes[i + 1]);
			}
			
  		}	
		return 1;
	}
	static int cm_setMessageParamAttributes(pool* p,char* xPath,int type,const char ** attributes,void* userdata){
		cm_parse_bundle* bundle=(cm_parse_bundle*)userdata;
		int i;
		
		for (i = 0; attributes[i]; i += 2) {
			if(strcmp(attributes[i],"name")==0){
				bundle->tmp=apr_pstrdup(p,attributes[i + 1]);
			}
  		}
		return 1;
	}
	static int cm_setMessageParamBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cm_parse_bundle* bundle=(cm_parse_bundle*)userdata;
		
		if(bundle->tmp!=NULL){
			apr_hash_set(bundle->msg->params,bundle->tmp,APR_HASH_KEY_STRING,apr_pstrdup(p,body));
			bundle->tmp=NULL;
		}
		
		return 1;
	}
	
	static int cm_setMessageBatchIdBody(pool* p,char* xPath,int type,const char *body,void* userdata){
		cm_parse_bundle* bundle=(cm_parse_bundle*)userdata;
		
		bundle->msg->batchId=apr_pstrdup(p,body);
		return 1;
	}
	cm_wire_message* cm_parseMessage(apr_pool_t* p,char* src){
		int result=0;
		cm_wire_message* msg=NULL;
		XmlCore* xCore;
		cm_parse_bundle bundle;
		
		if(src==NULL) return NULL;
		
		xCore=xmlcore_getXmlCore(p);
		msg=cm_newWireMessage(p,NULL,NULL);
		
		(&bundle)->tmp=NULL;
		(&bundle)->msg=msg;
		
		xmlcore_addXPathHandler(xCore,"/message",0,cm_setMessageAttributes,NULL,NULL, &bundle);
		xmlcore_addXPathHandler(xCore,"/message/param",0,cm_setMessageParamAttributes,cm_setMessageParamBody,NULL, &bundle);
		xmlcore_addXPathHandler(xCore,"/message/batchId",0,NULL,cm_setMessageBatchIdBody,NULL, &bundle);
		result=xmlcore_parseFromStringSource(xCore,src);
		
		//validate
		if(result>0&&msg->type==NULL){
			msg=NULL;
		}
		return msg;	
	}
	
	char* cm_serializeMessage(apr_pool_t* p, cm_wire_message* msg){
		apr_hash_index_t *hi;
		char* name=NULL,*value=NULL;
		char* ret=NULL;
		apr_pool_t* subp=NULL;
		

		if(msg==NULL) return NULL;
		if(apr_pool_create(&subp,p)!=APR_SUCCESS){
			return NULL;	
		}
		ret=apr_pstrcat(subp,"<message type=\"",msg->type,"\"",NULL);
		if(msg->header!=NULL){
			if(msg->header->nodeName!=NULL){
				ret=apr_pstrcat(subp,ret," node=\"",msg->header->nodeName,"\"",NULL);	
			}
			if(msg->header->nameSpace!=NULL){
				ret=apr_pstrcat(subp,ret," namespace=\"",msg->header->nameSpace,"\"",NULL);	
			}	
			if(msg->header->ipList!=NULL){
				ret=apr_pstrcat(subp,ret," ips=\"",msg->header->ipList,"\"",NULL);
			}
			if(msg->header->componentId!=NULL){
				ret=apr_pstrcat(subp,ret," com=\"",msg->header->componentId,"\"",NULL);
			}
			if(msg->header->version!=NULL){
				ret=apr_pstrcat(subp,ret," ver=\"",msg->header->version,"\"",NULL);
			}
		}
		ret=apr_pstrcat(subp,ret,">\n",NULL);
		if(msg->batchId!=NULL){
			ret=apr_pstrcat(subp,ret,"\t<batchId><![CDATA[",msg->batchId,"]]></batchId>\n",NULL);
		}
		for (hi = apr_hash_first(subp, msg->params); hi; hi = apr_hash_next(hi)) {
        	apr_hash_this(hi,(const void**)&name, NULL, (void**)&value);
        	ret=apr_pstrcat(subp,ret,"\t<param name=\"",name,"\"><![CDATA[",value,"]]></param>\n",NULL);
        }
        
        //make last concat to non temporary memory pool
		ret=apr_pstrcat(p,ret,"</message>",NULL);
		//destroy temporary memeory pool
		apr_pool_destroy(subp);
		return ret;        		
	}
	
	/**
	 * End Message Code
	 */
#endif
