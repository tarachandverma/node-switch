#ifndef __WSJACL_SHM_DATA__C_
#define __WSJACL_SHM_DATA__C_

#include <shm_data.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
//#include <search_utils.h>
#include <shm_dup.h>


        static int getSheapSpace(shared_heap* sheap){
                shared_page* page;
		if(sheap==NULL){return -1;}
		page=sheap->page;
                return page->segmentsize-(page->cursor-page->data)+(page->segmentsize*page->backsegment);
        }


	int shmdata_syncself(pool* p, shared_heap* sheap, rfunc function, void* userdata){
		APACHE_LOG_DEBUG("SHMCLIENT CHECK SYNC");
		if(sheap!=NULL&&sheap->page!=NULL){
			APACHE_LOG_DEBUG("SHMCLIENT CHECK SYNC1");
			if(sheap->timestamp!=sheap->page->timestamp){
				APACHE_LOG_DEBUG("CLIENT REFRESH");
				if(!shmutex_isRefreshing(sheap->refresh_mutex)){
					if(shmutex_getLock(sheap->refresh_mutex)){				
						sheap->local_segment=sheap->page->frontsegment;
						sheap->timestamp=sheap->page->timestamp;
						sheap->flipcount++;

						//do attach logic
						APACHE_LOG_DEBUG("Attaching to new update");
						if(function!=NULL){
							(*function)(p,sheap,userdata);
						}
						shmutex_unLock(sheap->refresh_mutex);
						return 2;
					}
				}
				return 1;		
			}
			APACHE_LOG_DEBUG("SHMCLIENT CHECK SYNC2");
		}else{
			return -1;
		}
	return 0;
	}

char* shmdata_32BitString_copy(shared_heap* sheap, char* str){
	int slen;
	char* nstr;
	
	if(str==NULL){
		return NULL;
	}
	slen=shmdup_32BitString_size(str);	
	nstr=shmdata_shpalloc(sheap,slen);
	memset(nstr,'\0',slen);
	memcpy(nstr,str,strlen(str));
	//APACHE_LOG_DEBUG("reach1");
return nstr;
}

// Copies source array of native data types to sheap array.
array_header* array_headerToSheap(shared_heap* sheap, array_header* sarray){
        array_header* dst;

        dst=(array_header*)shmdata_shpalloc(sheap, sizeof(array_header));
        memcpy(dst,sarray,sizeof(array_header));

        if(sarray->elts!=NULL){
                dst->elts=shmdata_shpalloc(sheap,(sarray->nelts*sarray->elt_size));
                memcpy(dst->elts,(void*)(sarray->elts), (sarray->nelts*sarray->elt_size));
        }else{
                dst->elts=NULL;
        }
	dst->pool=NULL;
return dst;
}

	shared_page* shmdata_getNewSharedPage(pool* p, apr_shm_t** shm_t, int segmentsize, char* path){
		int overheadsize,usersize, x;
		shared_page* page;
		apr_status_t rv;
		overheadsize=sizeof(shared_page);
		usersize=segmentsize*2;
			
		rv=apr_shm_create(shm_t,overheadsize+usersize,path,p);
#ifdef WIN32
		if(rv==APR_EEXIST) { // try to attach it; seems windows specific behaviour
			rv = apr_shm_attach(shm_t,path,p);
		}
#endif
		if(rv!=APR_SUCCESS){
			//if failure then try to remove shm segment and try again
			if(apr_shm_attach(shm_t,path,p)==APR_SUCCESS){
				apr_shm_destroy(*shm_t);
				rv=apr_shm_create(shm_t,overheadsize+usersize,path,p);
				//if again failure..possible bad shm file...remove and retry
				if(rv!=APR_SUCCESS){
					rv=apr_file_remove(path,p);
					if(rv==APR_SUCCESS){
						rv=apr_shm_create(shm_t,overheadsize+usersize,path,p);
					}
				}
			}else{ //if cannot attach blow file away and try again
				rv=apr_file_remove(path,p);
				if(rv==APR_SUCCESS){
					rv=apr_shm_create(shm_t,overheadsize+usersize,path,p);
				}	
			}
		}
		
		if(rv==APR_SUCCESS){
			APACHE_LOG_DEBUG1("SHARED PAGES CREATED: Path=%s",path);
			page=apr_shm_baseaddr_get(*shm_t);
			page->itemmax=MAX_PAGE_ITEMS;
			page->segmentsize=segmentsize;
			page->timestamp=SHM_TIMESTAMP_INIT;
			page->flipcount=0;
			page->frontsegment=1;
			page->backsegment=0;
			page->data=(char*)(page+1);
			page->cursor=page->data;
			for(x=0;x<SEGMENTS_PER_PAGE;x++){
				page->segments[x].itemcount=0;
			}			
			return page;
		}
		APACHE_LOG_DEBUG("SHARED PAGE BAD");
	return NULL;
	}

	static shared_page* shmdata_attachToSharedPage(pool* p, apr_shm_t** shm_t, int segmentsize, char* path){
		int overheadsize,usersize, x;
		shared_page* page;
		apr_status_t rv;
		overheadsize=sizeof(shared_page);
		usersize=segmentsize*2;
			
		rv = apr_shm_attach(shm_t,path,p); 
		if(rv!=APR_SUCCESS){
			APACHE_LOG_DEBUG("UNABLE TO ATTACH TO SHARED PAGE BAD");
		}
		
		if(rv==APR_SUCCESS){
			APACHE_LOG_DEBUG1("SHARED PAGES CREATED: Path=%s",path);
			page=apr_shm_baseaddr_get(*shm_t);
			page->itemmax=MAX_PAGE_ITEMS;
			page->segmentsize=segmentsize;
			page->timestamp=SHM_TIMESTAMP_INIT;
			page->flipcount=0;
			page->frontsegment=1;
			page->backsegment=0;
			page->data=(char*)(page+1);
			page->cursor=page->data;
			for(x=0;x<SEGMENTS_PER_PAGE;x++){
				page->segments[x].itemcount=0;
			}			
			return page;
		}
		APACHE_LOG_DEBUG("SHARED PAGE BAD");
	return NULL;
	}	

	shared_heap* shmdata_sheap_make(pool* p, int segmentSize, char* path){
		int x;
		shared_heap* sheap;
		apr_shm_t* shm_t;
		//struct shmid_ds ds;
		
		sheap=(shared_heap*)apr_palloc(p,sizeof(shared_heap));
		sheap->page=shmdata_getNewSharedPage(p,&shm_t,segmentSize,path);
		
		sheap->timestamp=-1;
		sheap->flipcount=0;
		sheap->local_segment=-1;		
		sheap->shm_main=shm_t;
		sheap->refresh_mutex=shmutex_getMutex(p);
		if(sheap->page==NULL){return NULL;}
	return sheap;
	}

	shared_heap* shmdata_sheap_attach(pool* p, int segmentSize, char* path){
		int x;
		shared_heap* sheap;
		apr_shm_t* shm_t;
		//struct shmid_ds ds;
		
		sheap=(shared_heap*)apr_palloc(p,sizeof(shared_heap));
		sheap->page=shmdata_attachToSharedPage(p,&shm_t,segmentSize,path);
		
		sheap->timestamp=-1;
		sheap->flipcount=0;
		sheap->local_segment=-1;		
		sheap->shm_main=shm_t;
		sheap->refresh_mutex=shmutex_getMutex(p);
		if(sheap->page==NULL){return NULL;}
	return sheap;
	}

	char* shmdata_getItem(shared_heap* sheap,const char* ID){
		int x;
		char* buf=NULL;
		segment_header* sh;

		sh=&(sheap->page->segments[sheap->local_segment]);	
		for(x=0;x<sh->itemcount;x++){
			if(strcmp(sh->items[x].ITEMID,ID)==0){
				buf=((char*)(sheap->page->data))+(sh->items[x].offset);	
				//printf("D:%ld, b: %ld, o:%d\n",sheap->page->data,buf, sh->items[x].offset);
				return buf;
			}
		}
	return buf;
	}
	int shmdata_getFlipCount(shared_heap* sheap){
		return sheap->page->flipcount;
	}
	time_t* shmdata_getLastFlipTime(shared_heap* sheap){
		return &(sheap->page->timestamp);
	}

	void shmdata_PublishBackSeg(shared_heap* sheap){
		int tmp;
		tmp=sheap->page->backsegment;
		sheap->page->backsegment=sheap->page->frontsegment;
		sheap->page->frontsegment=tmp;
		sheap->page->flipcount++;
		APACHE_LOG_DEBUG1("PUBLISH PAGE: %d",sheap->page->flipcount);
		sheap->page->timestamp=time(NULL);
	}
	void shmdata_rollback(shared_heap* sheap){
		int x,y;
		page_item* item;
		shared_page* p;
		segment_header* sh;

		APACHE_LOG_DEBUG("ROLLBACK BACKBUFFER SHEAP");

		p=sheap->page;
		sh=&(sheap->page->segments[sheap->page->backsegment]);

		if(sh->itemcount>=MAX_PAGE_ITEMS){
			y=MAX_PAGE_ITEMS-1;
		}else{
			y=sh->itemcount;
		}
		for(x=0;x<=y;x++){			
			item=&(sh->items[x]);
			memset(item->ITEMID,'\0',PAGE_ITEM_CHARS);	
			memset(item->INFO,'\0',PAGE_ITEM_CHARS);
		}
		p->cursor=p->data+(p->backsegment*p->segmentsize);
		sh->itemcount=0;
	}
	void shmdata_BeginTagging(shared_heap* sheap){
		shared_page* p=sheap->page;		
		p->cursor=p->data+(p->backsegment*p->segmentsize);
		APACHE_LOG_DEBUG1("BEGIN TAGGING @ %d",p->cursor);
		p->segments[p->backsegment].itemcount=0;
	}
	void shmdata_OpenItemTag(shared_heap* sheap,const char* ID){
		segment_header* sh=&(sheap->page->segments[sheap->page->backsegment]);
		page_item* item;
		item=&(sh->items[sh->itemcount]);
		memset(item->ITEMID,'\0',PAGE_ITEM_CHARS);
		memset(item->INFO,'\0',PAGE_ITEM_CHARS);
		strcpy(item->ITEMID,ID);	
		item->offset=sheap->page->cursor-sheap->page->data;
	}
	void shmdata_AppendInfo(shared_heap* sheap,char* info){
		segment_header* sh=&(sheap->page->segments[sheap->page->backsegment]);
		page_item* item=&(sh->items[sh->itemcount]);
		strcat(item->INFO,info);
	}
	void shmdata_CloseItemTag(shared_heap* sheap){
		segment_header* sh=&(sheap->page->segments[sheap->page->backsegment]);
		page_item* item;
		item=&(sh->items[sh->itemcount]);
		item->size=sheap->page->cursor-sheap->page->data-item->offset;		
		sh->itemcount++;
	}
	void shmdata_CloseItemTagWithInfo(shared_heap* sheap, char* info){
		segment_header* sh=&(sheap->page->segments[sheap->page->backsegment]);
		page_item* item;	
		item=&(sh->items[sh->itemcount]);
		strncpy(item->INFO,info,PAGE_ITEM_CHARS-1);
		shmdata_CloseItemTag(sheap);
	}
	int shmdata_cursor(shared_heap* sheap){
			if(sheap==NULL||sheap->page==NULL){
				return -1;	
			}
			return sheap->page->cursor-sheap->page->data;
	}
	char* shmdata_shpalloc(shared_heap* sheap, int size){
                shared_page* page;
                char* retchar;
                int spaceleft;

                page=sheap->page;
		spaceleft=page->segmentsize-(page->cursor-page->data)+(page->segmentsize*page->backsegment);
                if(spaceleft<size){
					APACHE_LOG_DEBUG("OUT OF SHEAP SPACE");
                    return NULL;
                }
				
                retchar=page->cursor;
                page->cursor+=size;
        return retchar;
    }

	char* shmdata_shpcalloc(shared_heap* sheap, int size){
		char* tmp;
		tmp=shmdata_shpalloc(sheap,size);
		memset(tmp,0,size);
		return tmp;
	}
	void* shmdata_memcpy(shared_heap* sheap, void* src, int size){
		void* ret=NULL;
		
		if(src!=NULL&&size>0){
			ret=shmdata_shpcalloc(sheap,size);
			memcpy(ret,src,size);
		}
		return ret;
	}
	
	static apr_sockaddr_t* shmdata_copyAddr1(shared_heap* sheap,apr_sockaddr_t* addr){
		apr_sockaddr_t* ret=NULL;
		ret=(apr_sockaddr_t*)shmdata_memcpy(sheap,addr,sizeof(apr_sockaddr_t));
		ret->hostname=shmdata_32BitString_copy(sheap,addr->hostname);
		ret->hostname=shmdata_32BitString_copy(sheap,addr->servname);
		ret->ipaddr_ptr=shmdata_memcpy(sheap,addr->ipaddr_ptr,addr->ipaddr_len);
		ret->next=NULL;
		return ret;
	}
	apr_sockaddr_t* shmdata_sockAddrCpy(shared_heap* sheap, apr_sockaddr_t* addr){
		apr_sockaddr_t* ret=NULL, *cur=NULL,*curShm=NULL;
		
		if(addr!=NULL){
			cur=addr;
			ret=curShm=shmdata_copyAddr1(sheap,addr);
			while(cur->next!=NULL){
				cur=cur->next;
				curShm->next=shmdata_copyAddr1(sheap,cur);
				curShm=curShm->next;
			}
		}
		
		return ret;
	}

#endif
