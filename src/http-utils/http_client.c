#ifndef HTTP_CLIENT_C_
#define HTTP_CLIENT_C_
#include <http-utils/http_client.h>

//curl stuff
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>

void http_init(){
	curl_global_init(CURL_GLOBAL_ALL);
}
char* http_getInfo(apr_pool_t* p){
	return curl_version();
}
void http_cleanup(){
		curl_global_cleanup();
}

int http_is200_OK(http_util_result* ret){
	return ret!=NULL&&ret->responseCode==200;	
}

int http_is404_NOT_FOUND(http_util_result* ret){
	return ret!=NULL&&ret->responseCode==404;	
}
static size_t http_memory_callback(void* ptr,size_t size, size_t nmemb, void* data){
	size_t realsize=size*nmemb;
	http_util_result* res=(http_util_result*)data;
	char* tmp=NULL;
	if(res->size<=0){
		res->size=realsize;
		res->data=apr_pcalloc(res->p,realsize+1);
		memcpy(res->data,ptr,realsize);
		//res->data=(char*)apr_pstrndup(res->p,ptr,realsize);
	}else{
		tmp=apr_pcalloc(res->p,res->size+realsize+1);
		memcpy(tmp,res->data,res->size);
		res->data=tmp;
		memcpy(tmp+res->size,ptr,realsize);
		res->size+=realsize;
		//res->data=(char*)apr_pstrcat(res->p,res->data,apr_pstrndup(res->p,(char*)ptr,realsize),NULL);
	}
	
	
	return realsize;
}

static http_util_result* http_createNewHttpUtilResult(apr_pool_t *p){
	http_util_result* ret=ret=apr_pcalloc(p,sizeof(http_util_result));
	ret->p=p;
	ret->data=NULL;
	ret->content_type=NULL;
	ret->size=0;
	ret->totalTime=0.0;
	ret->responseCode=-1;
	ret->headers_out=NULL;
	return ret;	
}


static void http_setBasicAuthOpts(CURL *curl_handle,char* userAndPass){
		curl_easy_setopt(curl_handle,CURLOPT_HTTPAUTH,CURLAUTH_BASIC);
		curl_easy_setopt(curl_handle,CURLOPT_USERPWD,userAndPass);
}


http_util_result* http_request_head_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass,char** error){
	CURL *curl_handle=NULL;
		CURLcode res;
		
		http_util_result* ret=http_createNewHttpUtilResult(p);		
		
		curl_handle=curl_easy_init();
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "HEAD");
		/*SSL Options*/ 
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
		
		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		
		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}
		
		//perform
		res=curl_easy_perform(curl_handle);
		if(error!=NULL&&res!=CURLE_OK&&res!=CURLE_PARTIAL_FILE){
			*error=apr_pstrdup(p,curl_easy_strerror(res));
		}

		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);	
		}		
		curl_easy_cleanup(curl_handle);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}

typedef struct header_cyc{
	struct curl_slist * slist;
	apr_pool_t* p;	
}header_cyc;

static int http_bind_headers(void *rec, const char *key, const char *value){
	header_cyc* hcyc=(header_cyc*)rec;
	char* val=NULL;
	if(key!=NULL){
		if(value!=NULL){
			val=apr_pstrcat(hcyc->p,key,":",value,NULL);
		}else{
			val=apr_pstrcat(hcyc->p,key,":",NULL);
		}
//		fprintf(stderr,"Header: %s\n",val);
//		fflush(stderr);
		hcyc->slist = curl_slist_append(hcyc->slist, val);
	}
	return 1;
}

typedef struct header_out_pack{
	apr_table_t* headers_out;
	apr_pool_t* pool;
	int cnt;
}header_out_pack;

static char* http_getTrimmedStr(apr_pool_t* p, char* str){
	char* start=NULL,*end=NULL;
	int len=0,size=0;
	
	if(str==NULL){
		return NULL;
	}
	
	start=str;
	while(*start!='\0'&&(*start==' '||*start=='\t'||*start=='\n'||*start=='\r')){
		start++;
	}
	len=strlen(str);
	end=&(str[len-1]);
	while(*end!='\0'&&(*end==' '||*end=='\t'||*end=='\n'||*end=='\r')){
		end--;
	}
	
	size=end-start+1;
	if(size<=0){return NULL;}
	return apr_pstrndup(p,start,size);
}

static int http_isContentChunked(apr_pool_t*p, char* contentType){
	char* tmp=http_getTrimmedStr(p,contentType);
	return (tmp!=NULL&&strcasecmp(tmp,"chunked")==0)?TRUE:FALSE;
}

static size_t http_bind_headers_out(void *ptr, size_t size, size_t nmemb, void *userdata){
	size_t retSize=size*nmemb;
	char* data=NULL;
	char* p1=NULL, *p2=NULL, *cursor=NULL, *cend=NULL;
	http_util_result* ret=(http_util_result*)userdata;
	
	
	data=(char*)apr_pstrndup(ret->p,ptr,retSize);
	cursor=strstr(data,":");

	if(cursor!=NULL){
		*cursor='\0';
		cursor++;
		cend=strstr(cursor,"\r");
		if(cend!=NULL){
			*cend='\0';
			//
			if(strcasecmp(data,"location")==0||strcasecmp(data,"set-cookie")==0
					||strcasecmp(data,"set-cookie2")==0||strcasecmp(data,"www-authenticate")==0
					||strcasecmp(data,"content-encoding")==0||strcasecmp(data,"vary")==0
					||strcasecmp(data,"content-disposition")==0
					||strcasecmp(data,"content-transfer-encoding")==0
					||(strcasecmp(data,"transfer-encoding")==0&&!http_isContentChunked(ret->p,cursor))){
				apr_table_setn(ret->headers_out,apr_pstrdup(ret->p,data),cursor);
			}else{
				apr_table_setn(ret->headers_out,apr_pstrcat(ret->p,"X-P-",data,NULL),cursor);		
			}
		}
	}
	return retSize;
}


static void http_setup_headerbind(apr_pool_t *p,CURL *curl_handle,http_util_result* ret,apr_table_t * headers_in){
	header_cyc headcyc;
	if(headers_in!=NULL){
			headcyc.p=p;
			headcyc.slist=NULL;
			apr_table_do(http_bind_headers,&headcyc,headers_in,NULL);
			if(headcyc.slist!=NULL){
				curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headcyc.slist);
			}
			
			//capture headers out
			ret->headers_out=apr_table_make(p,8);
			curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, http_bind_headers_out);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER,ret); 
	}	
}

http_util_result* http_request_get_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass, apr_table_t * headers_in,char** error){
		CURL *curl_handle=NULL;
		CURLcode res;
//		header_cyc headcyc;
//		header_out_pack houtp;
//		apr_table_t* headers_out=NULL;
		http_util_result* ret=http_createNewHttpUtilResult(p);		
		
		curl_handle=curl_easy_init();
//		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5);
		http_setup_headerbind(p,curl_handle,ret,headers_in);

		
		/*SSL Options*/ 
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
		
		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		
		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}
		
		//perform
		res=curl_easy_perform(curl_handle);
		if(error!=NULL&&res!=CURLE_OK){
			*error=apr_pstrdup(p,curl_easy_strerror(res));
		}
		
		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);	
		}
		
		curl_easy_cleanup(curl_handle);
//		fprintf(stderr,"Response: %d\n",ret->responseCode);
//		fprintf(stderr,"ResponseSize: %d\n",ret->size);
//		fflush(stderr);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}
http_util_result* http_request_get_verbose2(apr_pool_t *p,char* uri,long timeout,long connectionTimeout,char* userColonPass, apr_table_t * headers_in,char** error){
		CURL *curl_handle=NULL;
		CURLcode res;
//		header_cyc headcyc;
//		header_out_pack houtp;
//		apr_table_t* headers_out=NULL;
		http_util_result* ret=http_createNewHttpUtilResult(p);		
		
		curl_handle=curl_easy_init();
//		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, connectionTimeout);
		http_setup_headerbind(p,curl_handle,ret,headers_in);

		
		/*SSL Options*/ 
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
		
		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		
		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}
		
		//perform
		res=curl_easy_perform(curl_handle);
		if(error!=NULL&&res!=CURLE_OK){
			*error=apr_pstrdup(p,curl_easy_strerror(res));
		}
		
		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);	
		}
		
		curl_easy_cleanup(curl_handle);
//		fprintf(stderr,"Response: %d\n",ret->responseCode);
//		fprintf(stderr,"ResponseSize: %d\n",ret->size);
//		fflush(stderr);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}
http_util_result* http_request_get(apr_pool_t *p,char* uri,long timeout){
	return http_request_get_verbose(p,uri,timeout,NULL,NULL,NULL);
}

//http_util_result* http_request_post_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass,const char* postData,int postDataLen, apr_table_t * headers_in){

http_util_result* http_request_post_verbose(apr_pool_t *p,char* uri,long timeout, long connectionTimeout, char* userColonPass,const char* postData,int postDataLen, apr_table_t * headers_in, char **error){
		CURL *curl_handle=NULL;
		CURLcode res;
		http_util_result* ret=http_createNewHttpUtilResult(p);		
		
		
		curl_handle=curl_easy_init();
		curl_easy_setopt(curl_handle, CURLOPT_POST,1);
		//curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
		if(postData!=NULL){
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS , postData) ;
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE , postDataLen) ;
//			fprintf(stderr,"PostSize: %d\n",postDataLen);
//			fflush(stderr);
			//curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE , strlen(postData)) ;
		}else{
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE , 0);
		}
		
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, connectionTimeout);
		
		http_setup_headerbind(p,curl_handle,ret,headers_in);
		
		/*SSL Options*/ 
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
		
		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		
		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}
		
		//perform
		res = curl_easy_perform(curl_handle);
		if(error!=NULL&&res!=CURLE_OK){
			*error=apr_pstrdup(p,curl_easy_strerror(res));
		}
		
		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);	
		}		
		curl_easy_cleanup(curl_handle);
//		fprintf(stderr,"Response: %d\n",ret->responseCode);
//		fprintf(stderr,"ResponseSize: %d\n",ret->size);
//		fflush(stderr);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}



typedef struct http_put_data{
	size_t len;
	char* data;
	char* cursor;
}http_put_data;

static http_put_data* http_newPutDataObj(apr_pool_t *p, char* str, int datalen){
	http_put_data* ret=NULL;
	ret=apr_palloc(p,sizeof(http_put_data));
	ret->len=datalen;
	ret->data=str;
	ret->cursor=str;
	return ret;	
} 

size_t http_put_callback(void *buffer, size_t size, size_t nmemb, void *userp){
	http_put_data* putData=(http_put_data*)userp;
	int copyBytes=0;
	
	//max size to out buffer can hold
    int maxbytes = size*nmemb;
    
    if(putData->len>0){
    	if(putData->len>maxbytes){
    		copyBytes=maxbytes;
    	}else{
    		copyBytes=putData->len;
    	}
    	memcpy(buffer,putData->cursor,copyBytes);
    	//printf("out:%s,%d", buffer,putData->len);
    	putData->cursor+=copyBytes;
    	putData->len-=copyBytes;    	
    	return copyBytes;
    }else{
    	return 0;	
    }
} 

http_util_result* http_request_put_verbose2(apr_pool_t *p,char* uri,long timeout,char* userColonPass, const char* putData,int putDataLen, apr_table_t * headers_in){
	CURL *curl_handle=NULL;
		CURLcode res;
		http_util_result* ret=http_createNewHttpUtilResult(p);		
		http_put_data* putObj=NULL;
	    		
		curl_handle=curl_easy_init();

		if(putData!=NULL){
			putObj=http_newPutDataObj(p,(char*)putData,putDataLen);
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION,http_put_callback);
			curl_easy_setopt(curl_handle, CURLOPT_READDATA,putObj);
			curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE,putObj->len);
		}else{
			curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE,0);
		}
		curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
		curl_easy_setopt(curl_handle, CURLOPT_PUT, TRUE);
		
		
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5);
		
		http_setup_headerbind(p,curl_handle,ret,headers_in);
		
		/*SSL Options*/ 
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
		
		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		
		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}
		
		//perform
		curl_easy_perform(curl_handle);
		
		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);	
		}		
		curl_easy_cleanup(curl_handle);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}
http_util_result* http_request_put_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass, char* putData){
		CURL *curl_handle=NULL;
		CURLcode res;
		http_util_result* ret=http_createNewHttpUtilResult(p);		
		http_put_data* putObj=NULL;
		//struct curl_slist *slist = NULL;
		//slist = curl_slist_append(slist, "Accept: text/xml");
	    //slist = curl_slist_append(slist, "Content-Type: text/xml");
	    //slist = curl_slist_append(slist, "Expect:"); 
	    		
		curl_handle=curl_easy_init();
		//curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
		//curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PUT");
		//curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, slist);
		if(putData!=NULL){
			putObj=http_newPutDataObj(p,putData,strlen(putData));
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION,http_put_callback);
			curl_easy_setopt(curl_handle, CURLOPT_READDATA,putObj);
			curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE,putObj->len);
		}else{
			curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE,0);
		}
		curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
		curl_easy_setopt(curl_handle, CURLOPT_PUT, TRUE);
		
		
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5);
		
		
		
		/*SSL Options*/ 
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
		
		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		
		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}
		
		//perform
		curl_easy_perform(curl_handle);
		
		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);	
		}		
		curl_easy_cleanup(curl_handle);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}
http_util_result* http_request_delete_verbose(apr_pool_t *p,char* uri,long timeout,char* userColonPass){
		CURL *curl_handle=NULL;
		CURLcode res;
		http_util_result* ret=http_createNewHttpUtilResult(p);		
		
		curl_handle=curl_easy_init();
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		
		/*SSL Options*/ 
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
		
		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		
		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}
		
		//perform
		curl_easy_perform(curl_handle);
		
		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);	
		}		
		curl_easy_cleanup(curl_handle);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}
//this function is exactly copied from "http_request_delete_verbose" in addition to more paras
http_util_result* http_request_delete_verbose2(apr_pool_t *p,char* uri,long timeout,long connectionTimeout,char* userColonPass, apr_table_t * headers_in,char** error){
		CURL *curl_handle=NULL;
		CURLcode res;
		http_util_result* ret=http_createNewHttpUtilResult(p);

		curl_handle=curl_easy_init();
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, connectionTimeout);
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		http_setup_headerbind(p,curl_handle,ret,headers_in);

		/*SSL Options*/
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);

		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);

		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}

		//perform
		res = curl_easy_perform(curl_handle);
		if(error!=NULL&&res!=CURLE_OK){
			*error=apr_pstrdup(p,curl_easy_strerror(res));
		}

		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);
		}
		curl_easy_cleanup(curl_handle);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}
http_util_result* http_request_method(apr_pool_t *p,const char* methodName,char* uri,long timeout,char* userColonPass,const char* data,int dataLen, apr_table_t * headers_in){
	CURL *curl_handle=NULL;
			CURLcode res;
			http_util_result* ret=http_createNewHttpUtilResult(p);		
			
			
			curl_handle=curl_easy_init();
			curl_easy_setopt(curl_handle, CURLOPT_POST,1);
			curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
			curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, methodName);
			if(data!=NULL){
				curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS , data) ;
				curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE , dataLen) ;
	//			fprintf(stderr,"PostSize: %d\n",postDataLen);
	//			fflush(stderr);
				//curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE , strlen(postData)) ;
			}else{
				curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE , 0);
			}
			
			curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,http_memory_callback);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,ret);
			curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
			curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
			curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
			curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5);
			
			http_setup_headerbind(p,curl_handle,ret,headers_in);
			
			/*SSL Options*/ 
			//Allow connect to server using self signed certificate
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
			
			//Allow connect to server with cert hostname (CN) not matching connect string
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
			
			if(userColonPass!=NULL){
				http_setBasicAuthOpts(curl_handle,userColonPass);
			}
			
			//perform
			curl_easy_perform(curl_handle);
			
			//get option info
			res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
			res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
			res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
			if(ret->content_type!=NULL){
				ret->content_type=apr_pstrdup(p,ret->content_type);	
			}		
			curl_easy_cleanup(curl_handle);
			fprintf(stderr,"Response: %d\n",ret->responseCode);
			fprintf(stderr,"Response: data: %s\n",ret->data);
			fprintf(stderr,"ResponseSize: %d\n",ret->size);
			fprintf(stderr,"ResponseSize STR: %d\n",strlen(ret->data));
			//fflush(stderr);
			if(ret!=NULL&&ret->responseCode!=0){
				return ret;
			}
			return NULL;
}

//HEAD request with response headers but no body.
http_util_result* http_request_head_verbose2(apr_pool_t *p, const char* uri,long timeout,char* userColonPass,char** error){
	CURL *curl_handle=NULL;
		CURLcode res;
		
		http_util_result* ret=http_createNewHttpUtilResult(p);		
		ret->headers_out=apr_table_make(p,8);
		curl_handle=curl_easy_init();
		curl_easy_setopt(curl_handle, CURLOPT_URL,uri);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, HTTP_USER_AGENT);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "HEAD");
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, http_bind_headers_out);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER,ret); 
		
		/*SSL Options*/ 
		//Allow connect to server using self signed certificate
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
		
		//Allow connect to server with cert hostname (CN) not matching connect string
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		
		if(userColonPass!=NULL){
			http_setBasicAuthOpts(curl_handle,userColonPass);
		}
		
		//perform
		res=curl_easy_perform(curl_handle);
		if(error!=NULL&&res!=CURLE_OK&&res!=CURLE_PARTIAL_FILE){
			*error=apr_pstrdup(p,curl_easy_strerror(res));
		}

		//get option info
		res = curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &(ret->totalTime));
		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &(ret->responseCode));
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &(ret->content_type));
		if(ret->content_type!=NULL){
			ret->content_type=apr_pstrdup(p,ret->content_type);	
		}		
		curl_easy_cleanup(curl_handle);
		if(ret!=NULL&&ret->responseCode!=0){
			return ret;
		}
		return NULL;
}

char* http_response_data(http_util_result* resp) { return resp->data; }
char* http_response_content_type(http_util_result* resp) { return resp->content_type; }
size_t http_response_size(http_util_result* resp) { return resp->size; }
double http_response_time(http_util_result* resp) { return resp->totalTime; }
long http_response_code(http_util_result* resp) { return resp->responseCode; }
apr_table_t* http_response_headers_out(http_util_result* resp) { return resp->headers_out; }

#endif
