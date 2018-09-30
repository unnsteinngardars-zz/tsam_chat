#include "server.h"

/**
 * Entry for the server
*/
int main(int argc, char const *argv[])
{
	Server server = Server();
	server.set_max_buffer(1024);
	server.run();
	return 0;
}
