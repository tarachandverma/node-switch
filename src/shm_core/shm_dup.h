#ifndef __WSJACL_SHM_DUP__H_
#define __WSJACL_SHM_DUP__H_

#include <sys/types.h>
#include <apache_mappings.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef __cplusplus
extern "C" {
#endif
	
void* array_headerToBuf(void* buf, array_header* sarray);
int shmdup_32BitString_size(char* str);
char* shmdup_32BitString_copy(char** icharbuf, char* str);

int shmdup_arrayheader_size(array_header* arr);

#ifdef __cplusplus
}
#endif
	
#endif
