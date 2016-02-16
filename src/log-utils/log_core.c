#ifndef __WSJACL_LOG_CORE_C_
#define __WSJACL_LOG_CORE_C_

#include <unistd.h>
#include <log-utils/log_core.h>

int logcore_openLogFile(apr_pool_t* p,char* logfile_name){
	
	if(logfile_name==NULL){
		//printf("\nLogfile name not specified \n");
		return FALSE;
		}
	rLogFile.status=apr_file_open(&rLogFile.logfile_hndl,logfile_name,APR_APPEND | APR_WRITE | APR_CREATE,APR_OS_DEFAULT,p);
	if(rLogFile.status!=APR_SUCCESS){
		//printf("\nFailed to open logfile - %s\n", logfile_name);
		}
	rLogFile.pid=getpid();
	rLogFile.logfile_name=apr_pstrdup(p,logfile_name);
	rLogFile.p=p;
	
	return rLogFile.status;
	}
	
int logcore_closeLogFile(){
	int cur_pid;
	
	cur_pid=getpid();
	if(rLogFile.status!=APR_SUCCESS||cur_pid!=rLogFile.pid){
		//printf("\nFailed to close logfile - %s\n", logfile_name);
		return FALSE;
		}
		apr_file_close(rLogFile.logfile_hndl);
		rLogFile.status=-1;
		return APR_SUCCESS;
	}
	
int logcore_truncateLogFile(){
	apr_status_t status=0;
	int cur_pid;
	
	cur_pid=getpid();
	if(rLogFile.status!=APR_SUCCESS||cur_pid!=rLogFile.pid){
		//printf("\nLogfile has not been opened.\n");
		return FALSE;
	}
	status=apr_file_trunc(rLogFile.logfile_hndl,0);
	//if(status!=APR_SUCCESS)
		//printf("\nFailed to truncate logfile - %s\n", rLogFile.logfile_name);
	
	return status;
}
	
int logcore_trimLogFile(){
	//To ensure that logfile size never grows to a crazy size
	struct stat file_stat;
	
	stat(rLogFile.logfile_name,&file_stat);
	if(file_stat.st_size > MAX_LOGFILE_SIZE){
		logcore_truncateLogFile();
		return TRUE;
	}
	return FALSE;
}
	
int logcore_printLog(const char* format,...){
	
	va_list ap;
	apr_size_t out_bytes;
	char logs[1024];
	apr_status_t status=0;
	int cur_pid;
	
	//Print to stdout to keep current behaviour
	va_start(ap,format);
	vfprintf(stdout,format,ap);
	va_end(ap);
	
	va_start(ap,format);
	vsprintf(logs,format,ap);
	va_end(ap);
	
	logcore_trimLogFile();
	cur_pid=getpid();
	
	if(rLogFile.status!=APR_SUCCESS||cur_pid!=rLogFile.pid){
		//printf("\nLogfile has not been opened by this pool.\n");
		return FALSE;
	}
	status=apr_file_write_full(rLogFile.logfile_hndl,logs,strlen(logs),&out_bytes);
	//if(status!=APR_SUCCESS)
		//printf("\nFailed to write logfile - %s\n", rLogFile.logfile_name);
	
	return status;
}
	
#endif

