#ifndef __WSJACL_DB_CORE__C_
#define __WSJACL_DB_CORE__C_

#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <apache_mappings.h>
#include "db_core2.h"
#include <common_utils.h>
#include "config_messaging2.h"

#define DB_SETUP_SCRIPT "PRAGMA auto_vacuum =1; \
		PRAGMA encoding=\"UTF-8\"; \
		DROP TABLE IF EXISTS remote_nodes; \
		DROP TABLE IF EXISTS node_meta; \
		DROP TABLE IF EXISTS error_logs; \
		DROP TABLE IF EXISTS msg_history; \
		CREATE TABLE remote_nodes( \
			id INTEGER PRIMARY KEY, \
			name TEXT, \
			name_space TEXT, \
			component TEXT, \
			version TEXT, \
			ips  TEXT, \
			last_batchid TEXT, \
			last_ping TEXT, \
			last_refresh TEXT, \
			notice TEXT \
		); \
		CREATE TABLE node_meta( \
			id INTEGER PRIMARY KEY, \
			rn_id INTEGER,	\
			name TEXT, \
			value TEXT, \
			template TEXT, \
			set_time TEXT \
		);	\
		CREATE TABLE error_logs( \
			id INTEGER PRIMARY KEY, \
			moduleId TEXT, \
			typeId TEXT, \
			msg TEXT, \
			set_time TEXT \
		);	\
		CREATE TABLE msg_history( \
			id INTEGER PRIMARY KEY, \
			rn_id INTEGER, \
			remote_user,	\
			remote_ip,		\
			from_node TEXT, \
			from_namespace TEXT, \
			msg_batch TEXT, \
			msg_time TEXT, \
			msg_type TEXT \
		);	\
	"
#define DB_INSERT_REMOTE_NODE		"INSERT INTO remote_nodes(name,component,version,name_space,ips,last_batchid,last_ping) values('%s','%s','%s','%s','%s','%s','%s');"
#define DB_UPDATE_REMOTE_NODE		"UPDATE remote_nodes SET ips='%s',last_batchid='%s', last_ping='%s', version='%s' WHERE name='%s' AND name_space='%s' AND component='%s';"
#define DB_UPDATE_REMOTE_NODE_NOBATCH		"UPDATE remote_nodes SET ips='%s', last_ping='%s', version='%s' WHERE name='%s' AND name_space='%s' AND component='%s';"
#define DB_COUNT_REMOTE_NODES		"SELECT id FROM remote_nodes WHERE name='%s' AND name_space='%s' AND component='%s';"
#define DB_UPDATE_REFRESH_COMPLETE	"UPDATE remote_nodes SET last_refresh='%s' WHERE name='%s' AND name_space='%s';"
#define DB_UPDATE_REFRESH_FAILED	"UPDATE remote_nodes SET last_refresh='FAILED:%s' WHERE name='%s' AND name_space='%s';"
#define DB_DELETE_NODE_META			"DELETE FROM node_meta WHERE rn_id=%d AND name='%s';"
#define DB_INSERT_NODE_META			"INSERT INTO node_meta(rn_id,name,value,set_time,template) values(%d,'%s','%s','%s','%s');"
#define DB_INSERT_NODE_META_BLANK	"INSERT INTO node_meta(rn_id,name,value,set_time,template) values(%d,'%s','%s','%s',NULL);"
#define DB_UPDATE_NOTICE			"UPDATE remote_nodes SET notice='%s' WHERE name='%s' AND name_space='%s';"
#define DB_UPDATE_NOTICE_BLANK		"UPDATE remote_nodes SET notice=NULL WHERE name='%s' AND name_space='%s';"
#define DB_RINSERT_ERROR_LOG		"INSERT INTO error_logs(moduleId,typeId,msg,set_time) values('%s','%s','%s','%s');"
#define DB_RINSERT_MSG_LOG		"INSERT INTO msg_history(rn_id,remote_user,remote_ip,from_node,from_namespace,msg_batch,msg_time,msg_type) values('%s','%s','%s','%s','%s','%s','%s','%s');"

#define TEMPLATE_ERROR		"error"

	char* dbcore_initializeRealmDatabase(apr_pool_t* p, const char *filename){
		int rc=0;
		sqlite3 *db=NULL;
		char* zErrMsg=NULL;
		char* ret=NULL;

		rc=sqlite3_open(filename,&db);
		if(rc){
			sqlite3_close(db);
			return apr_pstrcat(p,"Can't open database:",filename,NULL);
		}

		//set world readable
		if(apr_file_perms_set(filename,APR_OS_DEFAULT|APR_WREAD)!=APR_SUCCESS){
#if WIN32
			printf("Could not make database {%s} world readable on windows : ",filename);
#else			
			return apr_pstrcat(p,"Could not make database world readable: ",filename,NULL);
#endif
		}

		rc=sqlite3_exec(db,DB_SETUP_SCRIPT,NULL,0,&zErrMsg);
		if(rc!=SQLITE_OK){
			ret=apr_pstrdup(p,sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
			sqlite3_close(db);
			return ret;
		}

		sqlite3_close(db);


		return NULL;
	}
	database_core* dbcore_bindToDatabaseCore(apr_pool_t* p,char* filename){
		int rc=0;
		database_core* ret=NULL;
		sqlite3 *db=NULL;

		rc=sqlite3_open_v2(filename,&db,SQLITE_OPEN_READWRITE,0);
		if(rc){
			sqlite3_close(db);
			return NULL;
		}
		ret=(database_core*)apr_palloc(p,sizeof(database_core));
		ret->db=db;
		return ret;
	}
	void dbcore_close(apr_pool_t* p,database_core* dbcore){
		if(dbcore!=NULL&&dbcore->db!=NULL){
			sqlite3_close(dbcore->db);
		}
	}
	static int remote_nodes_count(void *userdata,int argc, char** argv, char** azColName){
		long* id=(long*)userdata;
		int i;
	 	 for(i=0; i<argc; i++){
		    if(argv[i]){
		    	(*id)=atol(argv[i]);
		    }
		 }
		return 0;
	}

	static void dbcore_registerMessageParams(pool* p,long id, sqlite3 *db,apr_hash_t* params){
		apr_hash_index_t *hi=NULL;
		char *name,*val;
		char* zErrMsg;
		int rc=0;
		time_t tnow=time(NULL);
		char query[768];
		char* templ=NULL;

		if(params!=NULL&&apr_hash_count(params)>0){
			for (hi = apr_hash_first(p, params); hi; hi = apr_hash_next(hi)) {
				apr_hash_this(hi,(const void**)&name, NULL, (void**)&val);

				if(name!=NULL&&name[0]=='!'){
					templ=TEMPLATE_ERROR;
					name=name+1;
				}

				//delete
				sprintf(query,DB_DELETE_NODE_META,id,name);
				rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
				if(rc!=SQLITE_OK){
					//printf("%s\n",	sqlite3_errmsg(db));
				}

				//insert
				if(templ!=NULL){
					sprintf(query,DB_INSERT_NODE_META,id,name,val,ctime(&tnow),templ);
					rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
				}else{
					sprintf(query,DB_INSERT_NODE_META_BLANK,id,name,val,ctime(&tnow));
					rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
				}

//				if(rc!=SQLITE_OK){
//					printf("%s\n",	sqlite3_errmsg(db));
//				}
			}
		}
	}
	void dbcore_updateNotice(pool* p,database_core* dbCore,cm_wire_message* msg,char* notice){
		sqlite3 *db=dbCore->db;
		char query[1024];
		int rc=0;
		char* zErrMsg;
		if(notice!=NULL){
			sprintf(query,DB_UPDATE_NOTICE,notice,msg->header->nodeName,msg->header->nameSpace);
			rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
//			if(rc!=SQLITE_OK){
//				printf("%s\n",	sqlite3_errmsg(db));
//			}
		}else{
			rc=sqlite3_exec(db,DB_UPDATE_NOTICE_BLANK,NULL,0,&zErrMsg);
//			if(rc!=SQLITE_OK){
//				printf("%s\n",	sqlite3_errmsg(db));
//			}
		}

	}
	char* dbcore_logError(pool *p, database_core* dbCore,char* module, char* mtype, char* msg){
		int rc=0;
		time_t tnow;
		char* zErrMsg;
		char query[1024];
		sqlite3 *db=dbCore->db;
		//printf("REGISTER MESSAGE!\n");
		tnow=time(NULL);
		sprintf(query,DB_RINSERT_ERROR_LOG,SAFESTR(module),SAFESTR(mtype),SAFESTR(msg),ctime(&tnow));
		rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
		if(rc){
			if(zErrMsg!=NULL){
				return apr_pstrdup(p,zErrMsg);
			}else{
				return apr_pstrdup(p,sqlite3_errmsg(db));
			}
		}else{
			return NULL;
		}
	}
	void dbcore_registerMessage(pool* p,database_core* dbCore, cm_wire_message* msg){
		long id=-1;
		int rc=0;
		char* zErrMsg;
		char query[1024];
		time_t tnow;



		//sqlite3_stmt* stmt=NULL;
		sqlite3 *db=dbCore->db;
		//printf("REGISTER MESSAGE!\n");
		sprintf(query,DB_COUNT_REMOTE_NODES,msg->header->nodeName,msg->header->nameSpace,msg->header->componentId);

		tnow=time(NULL);
		//printf("SELECT %s\n",ctime(&tnow));
		rc=sqlite3_exec(db,query,remote_nodes_count,&id,&zErrMsg);
		if(rc!=SQLITE_OK){
			//printf("%s\n",	sqlite3_errmsg(db));
		}else{
			if(id<0){ //insert
				//printf("INSERT %s\n",ctime(&tnow));
				sprintf(query,DB_INSERT_REMOTE_NODE,msg->header->nodeName,msg->header->componentId,msg->header->version,msg->header->nameSpace,msg->header->ipList,SAFESTRBLANK(msg->batchId),ctime(&tnow));
				//printf("%s\n",	query);
				rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
				//if(rc!=SQLITE_OK){printf("%s\n",	zErrMsg);}
			}else{	//update
				//printf("UPDATE %s\n",ctime(&tnow));
				if(!cm_isAliveMessage(msg)){
					sprintf(query,DB_UPDATE_REMOTE_NODE,msg->header->ipList,SAFESTRBLANK(msg->batchId),ctime(&tnow),msg->header->version,msg->header->nodeName,msg->header->nameSpace,msg->header->componentId);
				}else{
					sprintf(query,DB_UPDATE_REMOTE_NODE_NOBATCH,msg->header->ipList,ctime(&tnow),msg->header->version,msg->header->nodeName,msg->header->nameSpace,msg->header->componentId);
				}
				//printf("%s\n",	query);
				rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
				//if(rc!=SQLITE_OK){printf("%s\n",	zErrMsg);}
			}
		}

		//deal with message params
		dbcore_registerMessageParams(p,id,db,msg->params);

		if(cm_isRefreshCompleteMessage(msg)){
			sprintf(query,DB_UPDATE_REFRESH_COMPLETE,ctime(&tnow),msg->header->nodeName,msg->header->nameSpace);
			rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
		}else if(cm_isRefreshFailedMessage(msg)){
			sprintf(query,DB_UPDATE_REFRESH_FAILED,ctime(&tnow),msg->header->nodeName,msg->header->nameSpace);
			rc=sqlite3_exec(db,query,NULL,0,&zErrMsg);
		}
		//dbcore_updateNotice(p,dbCore,msg,"TEST");

		//printf("REGISTER MESSAGE-Exit!\n");
	}
#endif
