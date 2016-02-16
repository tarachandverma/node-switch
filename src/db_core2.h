#ifndef __WSJACL_DB_CORE__H_
#define __WSJACL_DB_CORE__H_

#include <sys/types.h>
#include <apache_mappings.h>
#include <sqlite3.h>
#include "config_messaging_parsing2.h"

	typedef struct database_core{
		sqlite3 *db;
	}database_core;
	char* dbcore_initializeRealmDatabase(apr_pool_t* p, const char *filename);
	database_core* dbcore_bindToDatabaseCore(apr_pool_t* p,char* filename);
	void dbcore_close(apr_pool_t* p,database_core* dbcore);
	void dbcore_registerMessage(pool* p,database_core* dbCore, cm_wire_message* msg);
	void dbcore_updateNotice(pool* p,database_core* dbCore,cm_wire_message* msg,char* notice);
	char* dbcore_logError(pool *p, database_core* dbCore,char* module, char* mtype, char* msg);
#endif

