#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_
#ifdef __cplusplus
	extern "C" {
#endif
//apr stuff
#include <apr_general.h>
#include <apr_pools.h>
#include <apr_strings.h>
#include <apr_tables.h>

#define HTTP_USER_AGENT	"libswitch-libcurl"
	typedef struct http_util_result {
	   char *data;
	   char *content_type; 
	   size_t size;
	   double totalTime;
	   long responseCode;
	   apr_pool_t *p;
	   apr_table_t* headers_out;
	}http_util_result;

	void http_cleanup();
	void http_init();
	http_util_result* http_request_post_verbose(apr_pool_t *p,char* uri,long timeout,long connectionTimeout,char* userColonPass,const char* postData,int postDataLen, apr_table_t * headers_in,char** error);
	http_util_result* http_request_get_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass, apr_table_t * headers_in,char** error);
	http_util_result* http_request_get_verbose2(apr_pool_t *p,char* uri,long timeout,long connectionTimeout,char* userColonPass, apr_table_t * headers_in,char** error);
	http_util_result* http_request_put_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass, char* putData);
	http_util_result* http_request_put_verbose2(apr_pool_t *p,char* uri,long timeout,char* userColonPass, const char* putData,int putDataLen, apr_table_t * headers_in);
	http_util_result* http_request_method(apr_pool_t *p,const char* methodName,char* uri,long timeout,char* userColonPass,const char* data,int dataLen, apr_table_t * headers_in);
	http_util_result* http_request_delete_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass);
	http_util_result* http_request_delete_verbose2(apr_pool_t *p,char* uri,long timeout,long connectionTimeout,char* userColonPass, apr_table_t * headers_in,char** error);
	http_util_result* http_request_head_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass,char** error);
	http_util_result* http_request_head_verbose2(apr_pool_t *p,const char* uri,long timeout,char* userColonPass,char** error);
	char* http_getInfo(apr_pool_t* p);
	http_util_result* http_request_get(apr_pool_t *p,char* uri,long timeout);
	int http_is200_OK(http_util_result* ret);
	int http_is404_NOT_FOUND(http_util_result* ret);
	char* http_response_data(http_util_result* resp);
	char* http_response_content_type(http_util_result* resp);
	size_t http_response_size(http_util_result* resp);
	double http_response_time(http_util_result* resp);
	long http_response_code(http_util_result* resp);
	apr_table_t* http_response_headers_out(http_util_result* resp);
	
#ifdef __cplusplus
	}
#endif	
#endif /*HTTP_CLIENT_H_*/
