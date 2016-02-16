#ifndef __LIBSWITCH_CONF_EXT__H_
#define __LIBSWITCH_CONF_EXT__H_

#ifdef  __cplusplus
extern "C" {
#endif
	
#define LS_SUCCESS				0
#define LS_FAILURE				1

typedef int ls_status_t;

ls_status_t libswitch_initializeLibSwitch(const char* configFile);
int libswitch_isSwitchEnabled(const char* moduleId, const char* switchName, const char* switchValue);

#ifdef  __cplusplus
}
#endif

#endif //#if __LIBSWITCH_CONF__H_
