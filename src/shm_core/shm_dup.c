#ifndef __WSJACL_SHM_DUP__C_
#define __WSJACL_SHM_DUP__C_

#include <string.h>
#include <shm_dup.h>
#include <math.h>

void* array_headerToBuf(void* buf, array_header* sarray){
	array_header* larry;
	
	larry=buf;
	memcpy(larry,sarray,sizeof(array_header));
	//printf("NELTS: %d\n",larry->nelts);
	buf=(void*)(larry+1);
	if(sarray->elts!=NULL){
		memcpy(buf,(void*)(sarray->elts), (sarray->nelts*sarray->elt_size));
		larry->elts=buf;
		buf=(void*)(((char*)buf)+(sarray->nelts*sarray->elt_size));
	}else{
		larry->elts=NULL;
	}
return buf;
}

int shmdup_32BitString_size(char* str){
	int tmp, mod;
	if(str==NULL) return 0;
	
	tmp=strlen(str)+1;
	mod=tmp%4;

	if(mod>0){
		tmp=tmp-mod+4;
	}
return tmp;
}
int shmdup_arrayheader_size(array_header* arr){
        int sz=0;
        sz+=sizeof(array_header);
        sz+=arr->elt_size*arr->nelts;
return sz;
}
/**
 * Copies str to icharbuf and returns the beginning of the new string. icharbuf is updated rollingly
 */
char* shmdup_32BitString_copy(char** icharbuf, char* str){
	int slen;
	char* buf;
	
	if(str==NULL){
		return NULL;
	}
	
	buf=*icharbuf;

	slen=shmdup_32BitString_size(str);	
        memset(buf,'\0',(int)slen);
        memcpy(buf,str, strlen(str));

	*icharbuf+=(int)slen;	
	
return buf;
}

#endif
