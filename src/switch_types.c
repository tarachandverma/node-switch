
#include "switch_types.h"

	const char* st_getSwitchTypeString(switch_type stype) {
		switch(stype) {
		case switch_type_boolean:
			return "boolean";
		case switch_type_long:
			return "long";
		case switch_type_string:
			return "string";
		case switch_type_regex:
			return "regex";
		case switch_type_long_array:
			return "longArray";
		case switch_type_string_array:
			return "stringArray";
		case switch_type_regex_array:
			return "regexArray";
		case switch_type_long_file:
			return "longFile";
		case switch_type_string_file:
			return "stringFile";
		case switch_type_regex_file:
			return "regexFile";
		default:
			return "unknown";
		}		
	}
