#ifndef __MM_LOGGING__C_
#define __MM_LOGGING__C_

#include <unistd.h>
#include <apache_mappings.h>
#include <apr_time.h>
#include <apr_file_info.h>
#include <log-utils/logging.h>
#include <apr_general.h>
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>

#define BUFFERSIZE 2048

	mm_logger* mmlog_getLogger(pool* p,char* path,long maxLogFileSizeMB){
		apr_status_t rc;
		if(path==NULL) return NULL;
		
		mm_logger* ret=(mm_logger*)apr_pcalloc(p,sizeof(mm_logger));
		ret->filepath=apr_pstrdup(p,path);
		ret->maxLogFileSizeMB=maxLogFileSizeMB;

		if((rc=apr_file_open(&(ret->file),path,APR_READ|APR_WRITE|APR_CREATE|APR_APPEND,APR_OS_DEFAULT,p))!=APR_SUCCESS){
			return NULL;
		}
		apr_thread_mutex_create(&(ret->mutex), APR_THREAD_MUTEX_UNNESTED, p);

		ret->pool=p;
		return ret;
	}
	
	
	void mmlog_log(mm_logger* log,const char* a_format, ...){
		va_list va;
		//int i=0;
		int size=2048;
		apr_time_t tnow;
		char tbuf[64];
		char buffer[BUFFERSIZE];
		memset(buffer, '\0', BUFFERSIZE);
		
		apr_thread_mutex_lock(log->mutex);

		if(log->maxLogFileSizeMB>0) mmlog_rotateLogFile(log);

		va_start(va, a_format);
		vsnprintf(buffer, BUFFERSIZE-1, a_format, va);
			
		tnow=apr_time_now();
		memset(tbuf,'\0',64);
		apr_ctime(tbuf,tnow);
		apr_file_printf(log->file,"%s mm.monitor.pid [%d] %s\r\n",tbuf,getpid(),buffer);
		va_end(va);
		apr_thread_mutex_unlock(log->mutex);
	}

	int mmlog_rotateLogFile(mm_logger* log){
		mm_logger* ltmp;
		apr_finfo_t finfo;
		apr_time_t tnow;
		apr_time_exp_t texp;
		apr_status_t st;
		apr_size_t tbuflen,tbufmax=64;
		char tbuf[64],*newPath;

		st=apr_stat(&finfo,log->filepath,APR_FINFO_SIZE,log->pool);

		if(finfo.size > log->maxLogFileSizeMB*1024*1024){
			apr_file_printf(log->file,"Monitor File Size [%dB], Max Value [%dB].\r\n",finfo.size,log->maxLogFileSizeMB*1024*1024);
			mmlog_closeLogger(log);
			log->file=NULL;

			tnow=apr_time_now();
			memset(tbuf,'\0',64);
			apr_time_exp_lt(&texp,tnow);
			apr_strftime(tbuf,&tbuflen,tbufmax,"%F-%H_%M_%S",&texp);
			newPath=apr_psprintf(log->pool,"%s.%s.%d",log->filepath,tbuf,texp.tm_usec);
			apr_file_rename(log->filepath,newPath,log->pool);
			ltmp=mmlog_getLogger(log->pool,log->filepath,log->maxLogFileSizeMB);
			log->file=ltmp->file;

			return TRUE;
		}
		return FALSE;
	}

	void mmlog_printf(mm_logger* log,const char* a_format, ...){
		va_list va;
		//apr_time_t tnow;
		//char tbuf[64];
		char buffer[BUFFERSIZE];
		memset(buffer, '\0', BUFFERSIZE);
		va_start(va, a_format);
		vsnprintf(buffer, BUFFERSIZE-1, a_format, va);
		apr_file_printf(log->file,"%s\r\n",buffer);
		va_end(va);
	}

	//Returns True if file successully closed.
	int mmlog_closeLogger(mm_logger* log){
		apr_status_t rc;
		if(log==NULL) return FALSE;

		apr_thread_mutex_destroy(log->mutex);

		if((rc=apr_file_close(log->file))!=APR_SUCCESS){
			return FALSE;
		}
		return TRUE;
	}

	void mmlog_setMaxFileSize(mm_logger* log,int maxLogFileSizeMB){
		log->maxLogFileSizeMB=maxLogFileSizeMB;
	}
#endif
