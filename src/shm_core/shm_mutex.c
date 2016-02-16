#ifndef __WSJACL_SHM_MUTEX__C_
#define __WSJACL_SHM_MUTEX__C_
#include <shm_mutex.h>
	shmutex_refresh_mutex* shmutex_getMutex(pool *p){
		shmutex_refresh_mutex* mutex;
		apr_status_t status;
		
		mutex=apr_palloc(p,sizeof(shmutex_refresh_mutex));
		mutex->isRefreshing=0;
		status = apr_thread_mutex_create(&(mutex->thread_mutex),APR_THREAD_MUTEX_DEFAULT, p);
		if (status != APR_SUCCESS) {
			return NULL;
		}
		return mutex;
	}
	
	int shmutex_getLock(shmutex_refresh_mutex* mutex){	
		apr_status_t status;	
		status=apr_thread_mutex_lock(mutex->thread_mutex);
		if(status==APR_SUCCESS){
			if(!(mutex->isRefreshing)){
				mutex->isRefreshing=1;
				apr_thread_mutex_unlock(mutex->thread_mutex);
				return 1;
			}
			apr_thread_mutex_unlock(mutex->thread_mutex);
		}
		return 0;
	}
	int shmutex_isRefreshing(shmutex_refresh_mutex* mutex){
		return mutex->isRefreshing;
	}
	void shmutex_unLock(shmutex_refresh_mutex* mutex){
		 mutex->isRefreshing=0;
	}


#endif
