#ifndef STRING_UTILITIES_H
#define STRING_UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

/**
 * Utilities for string and vector functionality
*/
namespace string_utilities
{
	void trim_cstr(char* cstr);
	std::string trim_string(const std::string &s, const std::string &delimiters = " \f\n\r\t\v");
	std::vector<std::string> split_by_delimeter(std::string string_buffer, std::string delimeter);
	std::vector<std::string> split_into_commands_and_body(std::string input_string);
}

#endif