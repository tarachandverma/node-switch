#ifndef __MODAUTH_STOMP_UTILS__C_
#define __MODAUTH_STOMP_UTILS__C_
#include <stomp-utils/stomp_utils.h>
#include <stomp-utils/stomp.h>
#include "apr_strings.h"

stomp_connection* stompu_newConnectionObj(apr_pool_t *pool){
	stomp_connection* conn=(stomp_connection*)apr_pcalloc(pool,sizeof(stomp_connection));
	conn->socket=NULL;
	return conn;
}

stomp_frame* stompu_newFrameObj(apr_pool_t *pool){
	stomp_frame* frame=(stomp_frame*)apr_pcalloc(pool,sizeof(stomp_frame));
	return frame;
}


char* stompu_connect(apr_pool_t *p,stomp_connection *conn,const char *hostname, int port,char* user, char* pass,apr_int32_t timeout){
	apr_status_t rc;
	stomp_frame frame;
	stomp_frame *rframe=NULL;
	rc=stomp_connect( conn, hostname, port, p,timeout);
	if(rc!=APR_SUCCESS){
		return apr_pstrdup(p,"Connection failed");	
	}
	
	frame.command = "CONNECT";
	frame.headers = apr_hash_make(p);
	if(user!=NULL){
		apr_hash_set(frame.headers, "login", APR_HASH_KEY_STRING, user);
	}    
	if(pass!=NULL){      
		apr_hash_set(frame.headers, "passcode", APR_HASH_KEY_STRING, pass);
	}          
	frame.body = NULL;
	rc = stomp_write(conn, &frame,p);
	if(rc!=APR_SUCCESS){
		return apr_pstrdup(p,"Could not send frame");	
	}
	//verify connected response
	
	rc = stomp_read(conn, &rframe, p);
	if(rc!=APR_SUCCESS||(rc==APR_SUCCESS&&strcmp(rframe->command,"CONNECTED")!=0)){
		stompu_disconnect(p,conn);
		return apr_pstrdup(p,"Failed to recieved connection verified frame");
	}
	
	return NULL;
}
apr_status_t stompu_subscribe(apr_pool_t *p,stomp_connection *conn,char* destination){
	apr_status_t rc;
	stomp_frame frame;
	
	frame.command = "SUBSCRIBE";
	frame.headers = apr_hash_make(p);
	apr_hash_set(frame.headers, "destination", APR_HASH_KEY_STRING, destination);      
	frame.body = NULL;
	rc = stomp_write(conn, &frame,p);
	return rc;	
}

apr_status_t stompu_unsubscribe(apr_pool_t *p,stomp_connection *conn,char* destination){
	apr_status_t rc;
	stomp_frame frame;
	
	frame.command = "UNSUBSCRIBE";
	frame.headers = apr_hash_make(p);
	apr_hash_set(frame.headers, "destination", APR_HASH_KEY_STRING, destination);      
	frame.body = NULL;
	rc = stomp_write(conn, &frame,p);
	return rc;	
}

apr_status_t stompu_sendMessage(apr_pool_t *p,stomp_connection *conn,char* destination,char* message){
	apr_status_t rc;
	stomp_frame frame;
	frame.command = "SEND";
	frame.headers = apr_hash_make(p);
	apr_hash_set(frame.headers, "destination", APR_HASH_KEY_STRING, destination);
	  
	frame.body = message;
	rc = stomp_write(conn, &frame,p);
	return rc;
}
char* stompu_disconnect(apr_pool_t *p,stomp_connection *conn){
	apr_status_t rc;
	stomp_frame frame;

	frame.command = "DISCONNECT";
	frame.headers = NULL;
	frame.body = NULL;

	rc = stomp_write(conn, &frame,p);
	rc=stomp_disconnect(conn);
	if(rc!=APR_SUCCESS){
		return apr_pstrdup(p,"Disconnect failed");	
	}
	return NULL;
}
#endif
