#ifndef __WSJACL_SHM_DATA__H_
#define __WSJACL_SHM_DATA__H_

#include <sys/types.h>
#include <apache_mappings.h>
#include <apr_shm.h>
#include <shm_mutex.h>

#ifdef __cplusplus
extern "C" {
#endif
	
#define MAX_PAGE_ITEMS		16	
#define PAGE_ITEM_CHARS		128	
#define SEGMENTS_PER_PAGE	2
//#define TOKEN_KEY_OFFSET	12


//#define SHM_BOOT_STATUS_UP_GREEN        1
//#define SHM_BOOT_STATUS_DOWN_BLUE	0
//#define SHM_BOOT_STATUS_FAILED_RED	-1

#define SHM_TIMESTAMP_INIT	-1
#define SHM_FAILHARD_SHEAP	16

	
	typedef struct page_item{
                int offset, size;
                char ITEMID[PAGE_ITEM_CHARS];
		char INFO[PAGE_ITEM_CHARS];
        }page_item;

	typedef struct segment_header{
		page_item items[MAX_PAGE_ITEMS];
		int itemcount;
	}segment_header;

	typedef struct shared_page{
		time_t timestamp;
		int segmentsize, flipcount;
		char *cursor, *data;
		int itemmax,frontsegment, backsegment;
		segment_header segments[SEGMENTS_PER_PAGE];		
	}shared_page;
	
	//stored per process
	typedef struct shared_heap{
		shared_page* page;
		apr_shm_t* shm_main;
		time_t timestamp;
		sig_atomic_t flipcount;
		sig_atomic_t local_segment;
		//for thread sync
		shmutex_refresh_mutex* refresh_mutex;		
	}shared_heap;

	typedef int (*rfunc) (pool*, shared_heap*, void*);
	int shmdata_syncself(pool* p, shared_heap* sheap, rfunc function, void* userdata);

	int shmdata_cursor(shared_heap* sheap);
	char* shmdata_shpalloc(shared_heap* sheap, int size);
	char* shmdata_shpcalloc(shared_heap* sheap, int size);
	void shmdata_BeginTagging(shared_heap* sheap);
	void shmdata_CloseItemTag(shared_heap* sheap);
	void shmdata_CloseItemTagWithInfo(shared_heap* sheap, char* info);
	void shmdata_OpenItemTag(shared_heap* sheap,const char* ID);
	void shmdata_AppendInfo(shared_heap* sheap,char* info);
	void shmdata_PublishBackSeg(shared_heap* sheap);
	void shmdata_rollback(shared_heap* sheap);
	int shmdata_getFlipCount(shared_heap* sheap);
	time_t* shmdata_getLastFlipTime(shared_heap* sheap);
	shared_heap* shmdata_sheap_make(pool* p, int segmentSize, char* path);
	shared_page* shmdata_getNewSharedPage(pool* p, apr_shm_t** shm_t, int segmentsize, char* path);
	shared_heap* shmdata_sheap_attach(pool* p, int segmentSize, char* path);
	char* shmdata_getItem(shared_heap* sheap,const char* ID);

	//sheap copy stuff
	array_header* array_headerToSheap(shared_heap* sheap, array_header* sarray);
	char* shmdata_32BitString_copy(shared_heap* sheap, char* str);
	apr_sockaddr_t* shmdata_sockAddrCpy(shared_heap* sheap, apr_sockaddr_t* addr);
	void* shmdata_memcpy(shared_heap* sheap, void* src, int size);
	
#ifdef __cplusplus
}
#endif

#endif
