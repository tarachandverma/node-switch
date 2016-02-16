#ifndef __WSJACL_XML_CORE__H_
#define __WSJACL_XML_CORE__H_
#include <apache_mappings.h>
	typedef struct llnode {
    	void* data;
    	struct llnode *next;
		struct llnode *prev;
  	}llnode;

	typedef struct DLinkedList{
		llnode *head;
		llnode *tail;
		int elts;
	}DLinkedList;

	typedef struct xmlcore_rec{
		DLinkedList* xpath_nodes;
		DLinkedList* xpath_handlers;
		pool* p;	
	}XmlCore;


	typedef int (*xfunc) (pool*,char*,int,const char **,void*);
	typedef int (*bfunc) (pool*,char*,int,const char *,void*);
	typedef int (*efunc) (pool*,char*,int,void*);

	typedef struct x_rec{ 
		char* path;
		DLinkedList* xpath_nodes;
		void* userdata;
		xfunc start_function;
		bfunc body_function;
		efunc end_function;
		char* body;
	}x_rec;
	typedef struct node_info_rec{
		char* name;
	}node_info;

	
	

	DLinkedList* xmlcore_getLinkedList(pool *p);
	int xmlcore_AddToTail(pool* p, DLinkedList* xCore, void* elt);
	void* xmlcore_peekTail(pool* p, DLinkedList* ll);
	XmlCore* xmlcore_getXmlCore(pool *p);
	int xmlcore_addXPathHandler(XmlCore* xCore, char* path, int options, const xfunc sfunction, const bfunc bfunction, const efunc efunction, void* udata);
	int xmlcore_beginParsing(XmlCore* xCore, char* file);
	int xmlcore_parseFromStringSource(XmlCore* xCore,char* source);
	char* xmlcore_beginParsingTextResponse(XmlCore* xCore, char* file);
	char* xmlcore_parseFromStringSourceTextResponse(XmlCore* xCore, char* source);
#endif
