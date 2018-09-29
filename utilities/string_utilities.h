#ifndef STRING_UTILITIES_H
#define STRING_UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>


namespace string_utilities
{
	void trim_cstr(char* cstr);
	inline std::string trim_string(const std::string &s, const std::string &delimiters = " \f\n\r\t\v");
	int verify_sub_command(char * command);
	std::vector<std::string> split_by_delimeter(std::string string_buffer, std::string delimeter);
	std::string split_into_commands_and_body(std::string input_string);
}

#endif