#include <common_utils.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef __sun
#include <ifaddrs.h>
#endif
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <apr_time.h>
#include <errno.h>

#include <apr_network_io.h>

#ifdef WSJACL_USE_LCC
#include <io.h>
#endif 

#define LENGTH 0x1024

char* com_getTimeDiffString(apr_pool_t* p, time_t tme1, time_t tme2){
	unsigned long ttime, seconds=0,minutes=0,hours=0, days=0;
    float tmptime;
	char* buf;
	
	ttime=difftime(tme1,tme2);
        if(ttime>0){
                seconds=ttime%60;
                minutes=(ttime/60)%60;
                hours=(ttime/3600)%24;
                days=(ttime/86400);
        }
		buf=apr_palloc(p,128);
		memset(buf,'\0',128);
        sprintf(buf,"%d days, %d hours, %d minutes, %d seconds",days,hours,minutes,seconds);
	return buf;
}

char* com_getErrorStr(apr_pool_t* p, apr_status_t rv){
	char* status=NULL;
	status=apr_palloc(p,128);
	return apr_strerror(rv, status,128);
}

char* com_getPathFromStatusLine(apr_pool_t* p, char* src){
	int x, slen, stage=0, len;
	char* start, *ret;
	
	slen=strlen(src);
	for(x=0;x<slen;x++){
		if(stage==0){
			if(src[x]!=' '){
				stage=1;
			}
		}else if(stage==1){
			if(src[x]==' '){
				stage=2;
			}
		}else if(stage==2){
			if(src[x]!=' '){
				start=&src[x];
				stage=3;
			}	
		}else if(stage==3){
			if(src[x]==' '){
				len=(&src[x]-start)+1;
				ret=(char*)apr_palloc(p,len);
				memset(ret,'\0',len);
				memcpy(ret,start,len-1);
				return ret;
			}
		}
		
	}	
}

char* getElement(apr_array_header_t* data, int element){
	 if(data!=NULL&&data->nelts>element&&element>=0){
     	return ((char**)data->elts)[element];
	 }
	 return NULL;
}

void** getElementRef(apr_array_header_t* data, int element){
	if(data!=NULL&&data->nelts>element&&element>=0){
     	return ((void**)data->elts)+element;
	}
	return NULL;
}

char* comu_getNodeDetails(apr_pool_t* p,unsigned int defaultHttpPort){
	int i;
	char* ret=NULL;

#ifndef __sun
	//for second ip get test
	struct ifaddrs *ifa = NULL, *ifp = NULL;
	socklen_t salen;
	char ip[512];
	
	if (getifaddrs (&ifp) >= 0){
		i=0;
		for (ifa = ifp; ifa&&ifa->ifa_addr; ifa = ifa->ifa_next){
			if (ifa->ifa_addr->sa_family == AF_INET)
				salen = sizeof (struct sockaddr_in);
			else if (ifa->ifa_addr->sa_family == AF_INET6)
				salen = sizeof (struct sockaddr_in6);
			else
				continue;
			
			if (getnameinfo (ifa->ifa_addr, salen,ip, sizeof (ip), NULL, 0, NI_NUMERICHOST) < 0){
				continue;
				}
				
			if(strcmp(ip,"127.0.0.1")!=0&&strcmp(ip,"::1")!=0){ //don't need to know localhost ip4 or ip6
				if(ifa->ifa_addr->sa_family == AF_INET){
					if(i==0)
						ret=apr_psprintf(p,"%s:%u",ip,defaultHttpPort);
					else
						ret=apr_psprintf(p,"%s,%s:%u",ret,ip,defaultHttpPort);
					}
				else{
					if(i==0)
						ret=apr_pstrdup(p,ip);
					else
						ret=apr_pstrcat(p,ret,",",ip,NULL);						
					}
				i++;
				}
			}
		 freeifaddrs (ifp);
		}
//#endif
#else
    //char buf[128];
	char *buf;
    apr_sockaddr_t *localsa = NULL;
	apr_sockaddr_t* sa_iter = NULL;
    char _bind_address[APRMAXHOSTLEN+1];
    apr_status_t status = apr_gethostname(_bind_address, APRMAXHOSTLEN, p);
    if(status == APR_SUCCESS) {
		status = apr_sockaddr_info_get(&localsa, _bind_address, APR_UNSPEC, 0, 0, p);
		if(status == APR_SUCCESS) {
			i=0;
			for(sa_iter=localsa; sa_iter; sa_iter = sa_iter->next) {
				//memset (buf, '\0', 128);

				//status =  apr_sockaddr_ip_getbuf(buf, 22, sa_iter); // APR 1
				status =  apr_sockaddr_ip_get(&buf, sa_iter); // APR 0
				if(status == APR_SUCCESS) {
					if(strcmp(buf,"127.0.0.1")!=0&&strcmp(buf,"::1")!=0){ //don't need to know localhost ip4 or ip6
						if(sa_iter->family == APR_INET){
							if(i==0) {
								ret=apr_psprintf(p,"%s:%u",buf,defaultHttpPort);
							}
							else {
								ret=apr_psprintf(p,"%s,%s:%u",ret,buf,defaultHttpPort);
							}
						}
						else {
							if(i==0) {
								ret=apr_pstrdup(p,buf);
							}
							else {
								ret=apr_pstrcat(p,ret,",",buf,NULL);
							}
						}
						printf ("ip : %s\n", ret);
					}
				}
			}
		}
    }
#endif

	return ret;
}

#define CTEM_PRE	"{"
#define CTEP_POST	"}"
	char* comu_templateString(apr_pool_t* p, char* src, apr_hash_t* vals){
		char* cpy=NULL;
		char* ret=NULL,*begin=NULL, *end=NULL, *token=NULL, *tval=NULL;
		if(src==NULL){ return NULL; }
	
		cpy=apr_pstrdup(p,src);
		
		begin=strstr(cpy,"{");
		while(begin!=NULL){
			
			//find end
			end=strstr(begin,"}");
			
			//do append
			if(end!=NULL){
				*begin='\0';
				*end='\0';
				token=begin+1;
				tval=vals!=NULL?SAFESTRBLANK(apr_hash_get(vals,token,APR_HASH_KEY_STRING)):"";
				if(ret==NULL){
					ret=apr_pstrcat(p,cpy,tval,NULL);
				}else{
					ret=apr_pstrcat(p,ret,cpy,tval,NULL);
				}
				begin=cpy=end+1;
			}else{
				begin++;
			}
			
			//continue
			begin=strstr(begin,"{");
		}
		if(cpy!=NULL&&*cpy!='\0'){
			ret=apr_pstrcat(p,ret,cpy,NULL);
		}
		return ret;
	}
	
	//returns lowercase string on pool.
	char* comu_strToLower(apr_pool_t*p, const char *str){
		char* s,*ret;
		if(str==NULL)	return NULL;
		
		ret=apr_pstrdup(p,str);
		for(s=ret; *s!='\0'; s++){
			if(('A' <= *s) && (*s <= 'Z')){
				*s = 'a' + (*s - 'A');
			}
		}
		return ret;
	}	

        time_t comu_dateStringToSeconds(const char* dateString){
        		struct tm tm;

        		if(dateString==NULL||strptime(dateString, "%a %b %d %H:%M:%S %Y", &tm)==0) return -1;
        		tm.tm_isdst=-1;//daylight saving
        		return mktime(&tm);
        	}
        
        time_t comu_dateStringToEpoch(const char* dateString){
                struct tm tm;

		if (dateString==NULL) return -1;
                memset(&tm,0, sizeof(struct tm));
                strptime(dateString,"%Y%m%d%H%M%SZ", &tm);
                tm.tm_isdst=-1;//daylight saving
                return mktime(&tm);
        }
        
	//Usage : comu_templateDateString
	// y, M, d, h, m, s: full year, Month, day, hour, minute, seconds
	// yy, MM, dd, hh, mm, ss: lower two digits of year, Month, day, hour, minute, seconds
	// MMM, ddd : names of month and day
        
	static char* comu_getCurrentDateByFormat(apr_pool_t* p, char* dateTemplate){
		apr_time_exp_t t;
		char*ret=NULL;
		int tmp;

		if ( dateTemplate==NULL ) return NULL;

		apr_time_exp_lt(&t, apr_time_now());

		char* cursor = dateTemplate;

		while(*cursor!='\0'){
			char* val=NULL;
			char c=*cursor;
			int cnt=0;

			//find consecutive character
			while( (*cursor=='y'||*cursor=='M'||*cursor=='d'||*cursor=='h'||*cursor=='m'||*cursor=='s') &&( *cursor==*(cursor+1) ) ){
				cursor++;cnt++;
			}
			cnt++;
			if(c=='y'||c=='M'||c=='d'||c=='h'||c=='m'||c=='s'){
				//printf("%c[%d]\n", c, cnt);
				switch(cnt){
				case 4:	// Year supported only
					if(c=='y'){
						val = apr_psprintf(p, "%04d", 1900+t.tm_year);
					}else{
						return dateTemplate;
					}
					break;

				case 3:	//MMM & ddd
					if(c=='M'){
						val = apr_psprintf(p, "%s", apr_month_snames[t.tm_mon]);
					}else if(c=='d'){
						val = apr_psprintf(p, "%s", apr_day_snames[t.tm_wday]);
					}else{
						return dateTemplate;
					}

					break;
				case 2:	//yy, MM, dd, hh, mm, ss
				case 1:	//y, M, d, h, m, s
					switch(c){
					case 'y':
						tmp=t.tm_year%100;	break;
					case 'M':
						tmp=1+t.tm_mon;	break;
					case 'd':
						tmp=t.tm_mday;	break;
					case 'h':
						tmp=t.tm_hour;	break;
					case 'm':
						tmp=t.tm_min;	break;
					case 's':
						tmp=t.tm_sec;	break;
					default:
						return dateTemplate;
					}
					if(cnt==2){
						val = apr_psprintf(p, "%02d", tmp);
					}else{
						val = apr_psprintf(p, "%d", tmp);
					}

					break;
				default:
					return dateTemplate;
				}
				if(ret==NULL){
					ret=apr_pstrdup(p, val);//append one char
				}else{
					ret=apr_pstrcat(p, ret, val, NULL);//append one char
				}
			}else{	//copy other chars ASIS
				if(ret==NULL){
					ret = apr_psprintf(p, "%c", c);
				}else{
					ret = apr_psprintf(p, "%s%c", ret, c);
				}
			}
			cursor++;
		}
		//printf("%s=%s\n",dateTemplate, ret);
		return ret;
	}

	// format is same asa apr_strftime
	/*
	%a	Abbreviated weekday name *	Thu
	%A	Full weekday name * 	Thursday
	%b	Abbreviated month name *	Aug
	%B	Full month name *	August
	%c	Date and time representation *	Thu Aug 23 14:55:02 2001
	%d	Day of the month (01-31)	23
	%H	Hour in 24h format (00-23)	14
	%I	Hour in 12h format (01-12)	02
	%j	Day of the year (001-366)	235
	%m	Month as a decimal number (01-12)	08
	%M	Minute (00-59)	55
	%p	AM or PM designation	PM
	%S	Second (00-61)	02
	%U	Week number with the first Sunday as the first day of week one (00-53)	33
	%w	Weekday as a decimal number with Sunday as 0 (0-6)	4
	%W	Week number with the first Monday as the first day of week one (00-53)	34
	%x	Date representation *	08/23/01
	%X	Time representation *	14:55:02
	%y	Year, last two digits (00-99)	01
	%Y	Year	2001
	%Z	Timezone name or abbreviation	CDT
	%%	A % sign	%
	*/
	char* comu_getCurrentDateByFormat2(apr_pool_t* p, const char* format){
		#define STR_SIZE	80
		apr_status_t status;
		apr_time_exp_t t;
		apr_size_t sz;
		char buf[STR_SIZE];

		if ( format==NULL ) return NULL;

		memset(buf, '\0', STR_SIZE);

		apr_time_exp_lt(&t, apr_time_now());
		status = apr_strftime(buf, &sz, STR_SIZE, format, &t);
		//assert (status == APR_SUCCESS );

		return apr_pstrdup(p, buf);
	}

	char* comu_templateDateString(apr_pool_t* p, char* src){
		char* cpy=NULL;
		char* ret=NULL,*begin=NULL, *end=NULL, *token=NULL, *tval=NULL;
		if(src==NULL){ return NULL; }

		cpy=apr_pstrdup(p,src);

		begin=strstr(cpy,"{");
		while(begin!=NULL){

			//find end
			end=strstr(begin,"}");

			//do append
			if(end!=NULL){
				*begin='\0';
				*end='\0';
				token=begin+1;
				tval=SAFESTRBLANK(comu_getCurrentDateByFormat2(p,token));
				if(ret==NULL){
					ret=apr_pstrcat(p,cpy,tval,NULL);
				}else{
					ret=apr_pstrcat(p,ret,cpy,tval,NULL);
				}
				begin=cpy=end+1;
			}else{
				begin++;
			}

			//continue
			begin=strstr(begin,"{");
		}
		if(cpy!=NULL&&*cpy!='\0'){
			ret=apr_pstrcat(p,ret,cpy,NULL);
		}
		return ret;
	}
	int getElementCount(apr_array_header_t* data){
		return (data!=NULL) ? data->nelts : 0;
	}
	apr_time_t comu_dateStringToAprTime(const char*dateString, const char* format){
		struct tm tm;
		char* val;
		if(val==NULL||format==NULL) return -1;

		if(val==NULL||format==NULL||strptime(val, format, &tm)==0) return -1;
		tm.tm_isdst=-1;//daylight saving
		return apr_time_from_sec(mktime(&tm));
	}

#define MAX_LONG_CHARS	32

	char* astru_serializeCsvFromLongArray(apr_pool_t* p, apr_array_header_t* arr){
		char* ret=NULL;
		char tmp[MAX_LONG_CHARS];
		int i=0, rsize=0;
		if(arr!=NULL&&arr->nelts>0){
			rsize=MAX_LONG_CHARS*arr->nelts;
			ret=apr_palloc(p,rsize);
			memset(ret,'\0',rsize);
			for(i=0;i<arr->nelts;i++){
				if(i==0){
					sprintf(tmp,"%d",(long)getElement(arr,i));
				}else{
					sprintf(tmp,",%d",(long)getElement(arr,i));
				}
				strcat (ret,tmp);
			}
		}
		return ret;
	}

	char* astru_getTrimmedStr(apr_pool_t* p, char* str){
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

	char* astru_serializeCsvFromStringArray(apr_pool_t* p, apr_array_header_t* arr){
		char* ret=NULL;
		int i=0;
		if(arr!=NULL){
			for(i=0;i<arr->nelts;i++){
				if(i==0){
					ret=apr_pstrdup(p,getElement(arr,i));
				}else{
					ret=apr_pstrcat(p,ret,",",getElement(arr,i),NULL);
				}
			}
		}
		return ret;
	}

