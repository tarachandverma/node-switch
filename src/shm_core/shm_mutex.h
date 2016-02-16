#ifndef __WSJACL_SHM_MUTEX__H_
#define __WSJACL_SHM_MUTEX__H_
#include <sys/types.h>
#include <apache_mappings.h>

#ifdef __cplusplus
extern "C" {
#endif
	
	//for static use
	typedef struct shmutex_refresh_mutex{
		apr_thread_mutex_t *thread_mutex;
		sig_atomic_t isRefreshing;		
	}shmutex_refresh_mutex;
	
	shmutex_refresh_mutex* shmutex_getMutex(pool *p);
	int shmutex_getLock(shmutex_refresh_mutex* mutex);
	void shmutex_unLock(shmutex_refresh_mutex* mutex);
	int shmutex_isRefreshing(shmutex_refresh_mutex* mutex);

#ifdef __cplusplus
}
#endif	

#endif
