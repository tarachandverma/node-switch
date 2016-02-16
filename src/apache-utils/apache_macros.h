#ifndef __WSJACL_APACHE_MACROS__H_
#define __WSJACL_APACHE_MACROS__H_

	#define str(s) #s
	#define PIDWRAP(msg) "INFO: ID>%d< " msg "\r\n" , getpid ()
	#define PIDWRAPC(msg) "CRITICAL: ID>%d< " msg "" , getpid ()

	#define AP_DECLARE(x)	x
	#define APLOG_MARK
	#define APLOG_ERR

	#define APACHE_LOG_DEBUG(msg){ /*printf("DEBUG: %s\n",msg);*/}
	#define APACHE_LOG_DEBUG1(msg, arg1) { /*printf("DEBUG: ");printf(msg,arg1);printf("\n");*/ }

#endif
