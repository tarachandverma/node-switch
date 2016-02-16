#ifndef __MM_LOGGING__H_
#define __MM_LOGGING__H_

#include <sys/types.h>
#include <apache_mappings.h>
#include <apr_thread_mutex.h>

typedef struct mm_logger{
		char* filepath;
		apr_file_t* file;
		apr_pool_t* pool;
		long maxLogFileSizeMB;
	    apr_thread_mutex_t *mutex;
	}mm_logger;

	mm_logger* mmlog_getLogger(pool* p,char* path,long maxLogFileSizeMB);
	
	void mmlog_log(mm_logger* log,const char* a_format, ...);
	void mmlog_printf(mm_logger* log,const char* a_format, ...);
	void mmlog_setMaxFileSize(mm_logger* log,int maxLogFileSizeMB);
	int mmlog_rotateLogFile(mm_logger* log);
	int mmlog_closeLogger(mm_logger* log);
#endif

