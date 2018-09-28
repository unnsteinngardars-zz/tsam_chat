#include "string_utilities.h"

inline std::string string_utilities::trim_string(const std::string &s, const std::string &delimiters)
{
	return s.substr(0, s.find_last_not_of(delimiters) + 1);
}

void string_utilities::trim_cstr(char * cstr)
{
	if (cstr != NULL)
	{
		int size = strlen(cstr);
		char temp[size];
		memset(temp, 0, size);
		strncpy(temp, cstr, size + 1);
		std::string str(cstr);
		std::string trimmed_str = trim_string(str);
		memset(cstr, 0, trimmed_str.length());
		strncpy(cstr, trimmed_str.c_str(), trimmed_str.length() + 1);
	}
	return;
}