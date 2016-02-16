#ifndef __WSJACL_SHM_APR__H_
#define __WSJACL_SHM_APR__H_
#include <sys/types.h>
#include <apache_mappings.h>
#include <shm_data.h>
#include <shm_datatypes.h>

#ifdef __cplusplus
extern "C" {
#endif
	
	/**
 	* Abstract type for hash tables.
 	*/
	typedef struct shmapr_hash_t shmapr_hash_t;

	/**
	* Abstract type for scanning hash tables.
	*/
	typedef struct shmapr_hash_index_t shmapr_hash_index_t;

	array_header* shmapr_array_make(shared_heap* sheap, int nelts, int elt_size);
	void * shmapr_array_push(shared_heap* sheap, array_header *arr);
	int shmapr_array_addElementInOrder(shared_heap* sheap, array_header *arr, shmd_orderfunc orderfunc, void* elem);
	char* shmapr_array_broadcastToLList(shared_heap* sheap, array_header *arr, shmd_broadfunc broadfunc);
	array_header* shmapr_parseStringArrayFromCsv(shared_heap* sheap, int arraySz, const char* delim, char* src);
	array_header* shmapr_parseLongArrayFromCsv(shared_heap* sheap, int arraySz, const char* delim, char* src);
	array_header* shmapr_copyStringArrayToSheap(shared_heap* sheap, array_header* sarray);
	
	shmapr_hash_t* shmapr_hash_make(shared_heap* sheap);
	void* shmapr_hash_get(shmapr_hash_t *ht,const void *key,apr_ssize_t klen);
	void shmapr_hash_set(shared_heap* sheap,shmapr_hash_t *ht,const void *key,apr_ssize_t klen,const void *val);
	shmapr_hash_index_t * shmapr_hash_first(apr_pool_t* pool, shmapr_hash_t *ht);
	shmapr_hash_index_t* shmapr_hash_next(shmapr_hash_index_t *hi);
	void shmapr_hash_this(shmapr_hash_index_t *hi,const void **key,apr_ssize_t *klen,void **val);
	unsigned int shmapr_hash_count(shmapr_hash_t *ht);
#ifdef __cplusplus
}
#endif

#endif
