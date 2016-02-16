#ifndef __MODAUTH_STOMP_UTILS__H_
#define __MODAUTH_STOMP_UTILS__H_
#include <stomp-utils/stomp.h>
#include "apr_general.h"

stomp_connection* stompu_newConnectionObj(apr_pool_t *pool);
stomp_frame* stompu_newFrameObj(apr_pool_t *pool);
char* stompu_connect(apr_pool_t *p,stomp_connection *conn,const char *hostname, int port,char* user, char* pass,apr_int32_t timeout);
apr_status_t stompu_subscribe(apr_pool_t *p,stomp_connection *conn,char* destination);
apr_status_t stompu_unsubscribe(apr_pool_t *p,stomp_connection *conn,char* destination);
apr_status_t stompu_sendMessage(apr_pool_t *p,stomp_connection *conn,char* destination,char* message);
char* stompu_disconnect(apr_pool_t *p,stomp_connection *conn);
#endif
