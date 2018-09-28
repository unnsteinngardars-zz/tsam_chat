#ifndef BUFFER_CONTENT_H
#define BUFFER_CONTENT_H

#include <string>

class BufferContent
{	
	private:
	int socket;
	std::string command;
	std::string sub_command;
	std::string body;
	public:
	BufferContent();
	void set_command(std::string command);
	void set_sub_command(std::string sub_command);
	void set_body(std::string body);
	std::string get_body();
	std::string get_command();
	std::string get_sub_command();
};

#endif