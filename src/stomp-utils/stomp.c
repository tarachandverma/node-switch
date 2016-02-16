/*
 *  Created by Tarachand verma on 01/04/14.
 *
 */
#include <stdlib.h>
#include <strings.h>
#include <stomp-utils/stomp.h>
#include <apr_version.h>

#define CHECK_SUCCESS_INVALIDATE_CONNECTION if( rc!=APR_SUCCESS ) {stomp_disconnect(connection); return rc; }
#define CHECK_SUCCESS_INVALIDATE_CONNECTION_RECV if( rc==APR_EOF) {stomp_disconnect(connection); return rc;}else if(rc==APR_TIMEUP){return rc;}

int stomp_connection_isValid(stomp_connection *connection){
	if(connection==NULL||connection->socket==NULL){return 0;}
	return 1;
}
/********************************************************************************
 *
 * Used to establish a connection
 *
 ********************************************************************************/
apr_status_t stomp_connect(stomp_connection *connection, const char *hostname, int port, apr_pool_t *pool, apr_int32_t timeout)
{
	apr_status_t rc;
	int socket_family;
	if(connection==NULL) return APR_EGENERAL;
	//stomp_connection *connection=NULL;
   
//	//
//	// Allocate the connection and a memory pool for the connection.
//	//
//	connection = apr_pcalloc(pool, sizeof(connection));
//	if( connection == NULL )
//		return APR_ENOMEM;

#define CHECK_SUCCESS if( rc!=APR_SUCCESS ) { return rc; }

	// Look up the remote address
	rc = apr_sockaddr_info_get(&(connection->remote_sa), hostname, APR_UNSPEC, port, 0, pool);
	CHECK_SUCCESS;
	
	// Create and Connect the socket.
	socket_family = connection->remote_sa->sa.sin.sin_family;
#if (APR_MAJOR_VERSION>=1)
	rc = apr_socket_create(&(connection->socket), socket_family, SOCK_STREAM, APR_PROTO_TCP, pool);
	CHECK_SUCCESS;
	int connectionTimeout = timeout;
	rc = apr_socket_opt_get(connection->socket, 32, &connectionTimeout);
	CHECK_SUCCESS;
	int keepAliveTimeout = 1;
	rc = apr_socket_opt_get(connection->socket, APR_SO_KEEPALIVE, &keepAliveTimeout);
	CHECK_SUCCESS;
#else
	rc = apr_socket_create_ex(&(connection->socket), socket_family, SOCK_STREAM, APR_PROTO_TCP, pool);
	CHECK_SUCCESS;
	rc = apr_setsocketopt(connection->socket, APR_SO_TIMEOUT, timeout);
	CHECK_SUCCESS;
	rc = apr_setsocketopt(connection->socket, APR_SO_KEEPALIVE, 1);
	CHECK_SUCCESS;
#endif
	rc=apr_socket_timeout_set(connection->socket,timeout);
	CHECK_SUCCESS;

    rc = apr_socket_connect(connection->socket, connection->remote_sa);
	CHECK_SUCCESS_INVALIDATE_CONNECTION;

   // Get the Socket Info
   rc = apr_socket_addr_get(&(connection->remote_sa), APR_REMOTE, connection->socket);
	CHECK_SUCCESS;
   rc = apr_sockaddr_ip_get(&(connection->remote_ip), connection->remote_sa);
	CHECK_SUCCESS;
   rc = apr_socket_addr_get(&(connection->local_sa), APR_LOCAL, connection->socket);
	CHECK_SUCCESS;
   rc = apr_sockaddr_ip_get(&(connection->local_ip), connection->local_sa);
	CHECK_SUCCESS;

   // Set socket options.
   //	rc = apr_socket_timeout_set( connection->socket, 2*APR_USEC_PER_SEC);
   //	CHECK_SUCCESS;

#undef CHECK_SUCCESS

	return rc;
}

apr_status_t stomp_disconnect(stomp_connection *connection)
{
   apr_status_t result, rc;
	//stomp_connection *connection = *connection_ref;

   if( connection== NULL||!stomp_connection_isValid(connection))
      return APR_EGENERAL;

	result = APR_SUCCESS;
   rc = apr_socket_shutdown(connection->socket, APR_SHUTDOWN_READWRITE);
	if( result!=APR_SUCCESS )
		result = rc;

   if( connection->socket != NULL ) {
      rc = apr_socket_close(connection->socket);
      if( result!=APR_SUCCESS )
         result = rc;
      connection->socket=NULL;
   }
	return rc;
}

/********************************************************************************
 *
 * Wrappers around the apr_socket_send and apr_socket_recv calls so that they
 * read/write their buffers fully.
 *
 ********************************************************************************/
apr_status_t stomp_write_buffer(stomp_connection *connection, const char *data, apr_size_t size)
{
   apr_status_t rc;
   apr_size_t remaining = size;
   if(!stomp_connection_isValid(connection)) return APR_EGENERAL;

   size=0;
	while( remaining>0 ) {
		apr_size_t length = remaining;

		rc = apr_socket_send(connection->socket, data, &length);
		CHECK_SUCCESS_INVALIDATE_CONNECTION

      data+=length;
      remaining -= length;
	}
	return APR_SUCCESS;
}

typedef struct data_block_list {
   char data[1024];
   struct data_block_list *next;
} data_block_list;

apr_status_t stomp_read_buffer(stomp_connection *connection, char **data, apr_pool_t *pool)
{

   apr_pool_t *tpool=NULL;
   apr_status_t rc;
   data_block_list *head=NULL, *tail=NULL;
   apr_size_t i=0;
   apr_size_t bytesRead=0;
   char *p=NULL;

   if(!stomp_connection_isValid(connection)) return APR_EGENERAL;

   rc = apr_pool_create(&tpool, pool);
   if( rc != APR_SUCCESS ) {
      return rc;
   }

   head = tail = apr_pcalloc(tpool, sizeof(data_block_list));
   if( head == NULL )
      return APR_ENOMEM;

#define CHECK_SUCCESS if( rc!=APR_SUCCESS ) { apr_pool_destroy(tpool);	return rc; }

   // Keep reading bytes till end of frame is encountered.
	while( 1 ) {

		apr_size_t length = 1;
      apr_status_t rc = apr_socket_recv(connection->socket, tail->data+i, &length);
      CHECK_SUCCESS_INVALIDATE_CONNECTION_RECV

      if( length==1 ) {
         i++;
         bytesRead++;

         // Keep reading bytes till end of frame
         if( tail->data[i-1]==0 ) {
            char endline[1];
            // We expect a newline after the null.
            rc=apr_socket_recv(connection->socket, endline, &length);
            CHECK_SUCCESS_INVALIDATE_CONNECTION_RECV
            if( endline[0] != '\n' ) {
               return APR_EGENERAL;
            }
            break;
         }

         // Do we need to allocate a new block?
         if( i >= sizeof( tail->data) ) {
            tail->next = apr_pcalloc(tpool, sizeof(data_block_list));
            if( tail->next == NULL ) {
               apr_pool_destroy(tpool);
               return APR_ENOMEM;
            }
            tail=tail->next;
            i=0;
         }
      }
	}
#undef CHECK_SUCCESS

   // Now we have the whole frame and know how big it is.  Allocate it's buffer
   *data = apr_pcalloc(pool, bytesRead+1);
   p = *data;
   if( p==NULL ) {
      apr_pool_destroy(tpool);
      return APR_ENOMEM;
   }

   // Copy the frame over to the new buffer.
   for( ;head != NULL; head = head->next ) {
      int len = bytesRead > sizeof(head->data) ? sizeof(head->data) : bytesRead;
      memcpy(p,head->data,len);
      p+=len;
      bytesRead-=len;
   }

   apr_pool_destroy(tpool);
	return APR_SUCCESS;
}


static char* stomp_marshallMessage(apr_pool_t* p, stomp_frame *frame){
	char* buf=NULL;
	apr_hash_index_t *i=NULL;
	const void *key=NULL;
	void *value=NULL;

	if(frame==NULL){return 0;}

	buf=apr_pcalloc(p,1024);
	//command + new line
	strcpy(buf,frame->command);
	strcat(buf,"\n");

	 // Write the headers
	if( frame->headers != NULL ) {
	      for (i = apr_hash_first(NULL, frame->headers); i; i = apr_hash_next(i)) {
	         apr_hash_this(i, &key, NULL, &value);
	         strcat(buf,(char*)key);
	         strcat(buf,":");
	         strcat(buf,(char*)value);
	         strcat(buf,"\n");
	      }
	}
	strcat(buf,"\n");
	if( frame->body != NULL ) {
		strcat(buf,frame->body);
	}
	strcat(buf,"\0\n");
	//printf(buf);
	//fflush(stdout);
	return buf;
}

/********************************************************************************
 *
 * Handles reading and writing stomp_frames to and from the connection
 *
 ********************************************************************************/

apr_status_t stomp_write(stomp_connection *connection, stomp_frame *frame, apr_pool_t* p) {
   apr_status_t rc;
   char* buf=NULL;

//   apr_hash_index_t *i=NULL;
//   const void *key=NULL;
//   void *value=NULL;
//
   char* msg=NULL;

   //build and write the frame
   msg=stomp_marshallMessage(p,frame);
   rc = stomp_write_buffer(connection, msg, strlen(msg)+2);
   return rc;
}

apr_status_t stomp_read(stomp_connection *connection, stomp_frame **frame, apr_pool_t *pool) {
	// Parse the header line.
   char *p2=NULL;
   void *key=NULL;
   void *value=NULL;

   apr_status_t rc;
   char *buffer=NULL;
   stomp_frame *f=NULL;

   f = apr_pcalloc(pool, sizeof(stomp_frame));
   if( f == NULL )
      return APR_ENOMEM;

   f->headers = apr_hash_make(pool);
   if( f->headers == NULL )
      return APR_ENOMEM;

#define CHECK_SUCCESS if( rc!=APR_SUCCESS ) { return rc; }

   // Read the frame into the buffer
   rc = stomp_read_buffer(connection, &buffer, pool);
   CHECK_SUCCESS;

   // Parse the frame out.
   {
      char *p;

      // Parse the command.
      p = strstr(buffer,"\n");
      if( p == NULL ) {
         // Expected at least 1 \n to delimit the command.
         return APR_EGENERAL;
      }

      // Null terminate the command.
      *p=0;
      f->command = buffer;
      buffer = p+1;

      while(1){ //parse headers

    	  p = strstr(buffer,"\n");
	       if( p == NULL ) {
	          // Expected at least 1 more \n to delimit the start of the body or end of header
	          return APR_EGENERAL;
	       }
	       //null terminate line
	       *p='\0';

	       if( (p-buffer) == 0 ) {
              // We have found the beginning of the body
              buffer = p+1;
              break;
           }

	       //parse header
	       p2 = strstr(buffer,":");
           if( p2 == NULL ) {
              // Expected at 1 : to delimit the key from the value.
              return APR_EGENERAL;
           }

           // Null terminate the key
           *p2=0;
           key = buffer;

           // The rest if the value.
           value = p2+1;

           // Insert key/value into hash table.
           apr_hash_set(f->headers, key, APR_HASH_KEY_STRING, value);


	       buffer = p+1;
      }

      // The rest of the buffer is the body.
      f->body = buffer;
   }

#undef CHECK_SUCCESS
   *frame = f;
	return APR_SUCCESS;
}

