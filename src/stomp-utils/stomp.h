/*
 *  Created by Tarachand verma on 01/04/14.
 *
 */
#ifndef STOMP_H
#define STOMP_H

#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_hash.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
   
typedef struct stomp_connection {
      apr_socket_t *socket;      
      apr_sockaddr_t *local_sa;
      char *local_ip;      
      apr_sockaddr_t *remote_sa;
      char *remote_ip;
} stomp_connection;

typedef struct stomp_frame {
   char *command;
   apr_hash_t *headers;
   char *body;
} stomp_frame;


int stomp_connection_isValid(stomp_connection *connection);
apr_status_t stomp_connect(stomp_connection *connection, const char *hostname, int port, apr_pool_t *pool, apr_int32_t timeout);
APR_DECLARE(apr_status_t) stomp_disconnect(stomp_connection *connection);

APR_DECLARE(apr_status_t) stomp_write(stomp_connection *connection, stomp_frame *frame, apr_pool_t *pool);
APR_DECLARE(apr_status_t) stomp_read(stomp_connection *connection, stomp_frame **frame, apr_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif  /* ! STOMP_H */
