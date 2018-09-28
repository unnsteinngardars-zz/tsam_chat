#include "buffer_content.h"

BufferContent::BufferContent()
{
	command = "";
	sub_command = "";
	body = "";
};

void BufferContent::set_command(std::string c)
{
	command = c;
}

void BufferContent::set_sub_command(std::string s)
{
	sub_command = s;
}

void BufferContent::set_body(std::string b)
{
	body = b;
}

std::string BufferContent::get_body()
{
	return body;
}

std::string BufferContent::get_command()
{
	return command;
}

std::string BufferContent::get_sub_command()
{
	return sub_command;
}

void BufferContent::set_file_descriptor(int fd)
{
	socket = fd;
}

int BufferContent::get_file_descriptor()
{
	return socket;
}

void BufferContent::set_sending_user(std::string su)
{
	sending_user = su;
}

std::string BufferContent::get_sending_user()
{
	return sending_user;
}

