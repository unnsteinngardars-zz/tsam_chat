#ifndef STRING_UTILITIES_H
#define STRING_UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>


namespace string_utilities
{
	void trim_cstr(char* cstr);
	inline std::string trim_string(const std::string &s, const std::string &delimiters = " \f\n\r\t\v");
	int verify_sub_command(char * command);
}

#endif