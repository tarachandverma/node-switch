#ifndef __WSJACL_LOG_CORE_H_
#define __WSJACL_LOG_CORE_H_

#include <apache_mappings.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#define MAX_LOGFILE_SIZE 51200

typedef struct logcore_logFile {
	
	char* logfile_name;
	int pid;
	apr_file_t* logfile_hndl;
	apr_status_t status;
	apr_pool_t* p;
}logcore_logFile;
	
static logcore_logFile rLogFile;
	
int logcore_openLogFile(apr_pool_t* p,char* logfile_name);
int logcore_closeLogFile();
int logcore_truncateLogFile();
int logcore_trimLogFile();
int logcore_printLog(const char* format,...);

#endif

