#ifndef SWITCH_TYPES_H_
#define SWITCH_TYPES_H_

typedef enum {
    switch_type_unknown = 'a',          	/* boolean type */
    switch_type_boolean = 'b',          	/* boolean type */
    switch_type_long = 'c',          		/* number type */
    switch_type_string = 'd',             	/* string type */
    switch_type_regex = 'e',             	/* regex type */	
    switch_type_long_array = 'f',
    switch_type_string_array = 'g',
    switch_type_regex_array = 'h',			
    switch_type_long_file = 'i',
    switch_type_string_file = 'j',
    switch_type_regex_file =  'k'   
} switch_type;

const char* st_getSwitchTypeString(switch_type stype);

#endif /*SWITCH_TYPES_H_*/
