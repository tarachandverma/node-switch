#ifndef __WSJACL_COMMON_UTILS__H_
#define __WSJACL_COMMON_UTILS__H_
#ifdef __cplusplus
	extern "C" {
#endif
#include <apr_general.h>
#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_hash.h>
#include <apr_strings.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <apr_thread_proc.h>

typedef struct com_message_list{
	apr_pool_t* p;
	apr_array_header_t* messages;
}com_message_list;

#define SAFESTR(str) (str!=NULL?str:"NULL")
#define SAFESTRBLANK(str) (str!=NULL?str:"") 
#define SAFESTRELSE(str,elstr) (str!=NULL?str:elstr) 
#define SAFESTRLEN(str) (str!=NULL?strlen(str):0)
#define BOOLTOSTR(bol) (bol!=1?"FALSE":"TRUE")
#define STRTOBOOL(str) ((str!=NULL&&(strcmp(str,"true")==0||strcmp(str,"TRUE")==0||strcmp(str,"on")==0))?1:0)
#define SAFEDUP(p,str) (str==NULL?NULL:apr_pstrdup(p,str))

// Use this macro instead of getElement 
#ifndef APR_ARRAY_IDX
	#define APR_ARRAY_IDX(ary,i,type) (((type *)(ary)->elts)[i])
#endif

// Use this macro instead of apr_array_push 
#ifndef APR_ARRAY_PUSH
	#define APR_ARRAY_PUSH(ary,type) (*((type *)apr_array_push(ary)))
#endif

// Use this macro instead of getElementRef 
#ifndef APR_ARRAY_REF_IDX
	#define APR_ARRAY_REF_IDX(ary,i,type) (((type *)(ary)->elts)+i)
#endif

// Use this macro instead of getElementCount
#ifndef APR_ARRAY_NUM_ELTS
	#define APR_ARRAY_NUM_ELTS(ary) ( (ary!=NULL) ? (ary)->nelts : 0 )
#endif

char* com_getErrorStr(apr_pool_t* p, apr_status_t rv);

int getElementCount(apr_array_header_t* data);

char* getElement(apr_array_header_t* data, int element);

void** getElementRef(apr_array_header_t* data, int element);

char* com_getPathFromStatusLine(apr_pool_t* p, char* src);

char* com_getTimeDiffString(apr_pool_t* p, time_t tme1, time_t tme2);
char* comu_getNodeDetails(apr_pool_t* p,unsigned int defaultHttpPort);
char* comu_templateString(apr_pool_t* p, char* src, apr_hash_t* vals);

com_message_list* com_newMessageList(apr_pool_t* p);
void com_messageListAddEntry(com_message_list* mList,char* message);
int com_messageListIsEmpty(com_message_list* mList);
char* comu_strToLower(apr_pool_t*p, const char* str);
time_t comu_dateStringToEpoch(const char* dateString);
time_t comu_dateStringToSeconds(const char* dateString);
char* comu_templateDateString(apr_pool_t* p, char* src);
apr_time_t comu_dateStringToAprTime(const char*dateString, const char* format);
char* comu_getCurrentDateByFormat2(apr_pool_t* p, const char* format);
char* astru_serializeCsvFromLongArray(apr_pool_t* p, apr_array_header_t* arr);
char* astru_getTrimmedStr(apr_pool_t* p, char* str);
char* astru_serializeCsvFromStringArray(apr_pool_t* p, apr_array_header_t* arr);
#ifdef __cplusplus
	}
#endif
#endif//__WSJACL_COMMON_UTILS__H_
