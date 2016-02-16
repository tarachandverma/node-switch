#ifndef __SHM_DATATYPES__C_
#define __SHM_DATATYPES__C_
#include <shm_datatypes.h>
#include <apache_mappings.h>

shmd_llist* shmd_list_create(shared_heap* sheap){
	shmd_llist* list;
	list=(shmd_llist*)shmdata_shpalloc(sheap,sizeof(shmd_llist));
	list->head=list->tail=NULL;
	list->elts;
}

static shmd_node* shmd_newShmdNode(shared_heap* sheap, void* elt){
	shmd_node* node;
	node=(shmd_node*)shmdata_shpalloc(sheap,sizeof(shmd_node));
	node->data=elt;
	node->prev=NULL;
	node->next=NULL;
	return node;
}

int shmd_list_addElementInOrder(shared_heap* sheap, shmd_llist* ll, shmd_orderfunc orderfunc, void* elem){
		shmd_node* node, *cursor, *tmp;
		int res;
		if(ll->head==NULL||ll->tail==NULL){					
			node=shmd_newShmdNode(sheap,elem);
			ll->head=ll->tail=node;
		}else{
			cursor=ll->head;
			while(cursor!=NULL){
				res=(*orderfunc)(sheap,cursor->data,elem);
				if(res>0){
					node=shmd_newShmdNode(sheap,elem);

					//element needs to be ahead of this one					
					if(cursor->prev==NULL){
						ll->head=node;
						node->next=cursor;
						cursor->prev=node;
					}else{
						tmp=cursor->prev;
						tmp->next=node;
						node->next=cursor;
						cursor->prev=node;
					}
					break;
				}else if(res==0){
					return -1;
				}
				cursor=cursor->next;
			}
			
			if(cursor==NULL){
				//add to end
				node=shmd_newShmdNode(sheap,elem);

				tmp=ll->tail;
				tmp->next=node;
				node->prev=tmp;
				ll->tail=node;
			}			
		}
		ll->elts++;
	return ll->elts;
}

char* shmd_list_broadcast(shared_heap* sheap, shmd_llist* ll, shmd_broadfunc broadfunc){
	shmd_node* node;

	node=ll->head;	
	while(node!=NULL){		
		(*broadfunc)(sheap,node->data);
		node=node->next;
	}
	return NULL;
}
#endif
