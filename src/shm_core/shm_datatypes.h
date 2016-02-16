#ifndef __SHM_DATATYPES__H_
#define __SHM_DATATYPES__H_
#include <shm_data.h>

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct shmd_node{
		void* data;
    	struct shmd_node *next;
		struct shmd_node *prev;
	}shmd_node;

	typedef struct shmd_llist{
		shmd_node *head;
		shmd_node *tail;
		int elts;
	}shmd_llist;

	typedef int (*shmd_orderfunc) (shared_heap*,void*,void*);
	typedef char* (*shmd_broadfunc) (shared_heap*,void*);
	
	shmd_llist* shmd_list_create(shared_heap* sheap);
	int shmd_list_addElementInOrder(shared_heap* sheap, shmd_llist* ll, shmd_orderfunc orderfunc, void* elem);
	char* shmd_list_broadcast(shared_heap* sheap, shmd_llist* ll, shmd_broadfunc broadfunc);

#ifdef __cplusplus
}
#endif
	
#endif
