#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char const *argv[])
{
	/* code */
	// 50001, 50002, 50003

	if (argc < 4)
	{
		error("invalid argument count, send in three ports to knock on as arguments");
	}

	int fd;
	struct sockaddr_in address;
	struct hotent* server;

	char buffer[1024];

	int first_port = atoi(argv[1]);
	int second_port = atoi(argv[2]);
	int third_port = atoi(argv[3]);

	

	return 0;
}
