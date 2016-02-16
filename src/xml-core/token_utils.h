#ifndef __WSJACL_TOKEN_UTILS__H_
#define __WSJACL_TOKEN_UTILS__H_
#include <apache_mappings.h>
typedef struct {
	char*	text;
	char*	delim;
	int	delim_len;
	size_t	offset;
	pool* p;
} Tokener;


Tokener* toku_getTokenizer (pool* p, char* text, char* delim);
char *toku_next_token(Tokener* tok);
char *toku_remaining_text (Tokener* tok);
#endif
