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


int main(int argc, char *argv[])
{
	/* code */
	// 50001, 50002, 50003
	if (argc < 5)
	{
		error("invalid argument count, send in three ports to knock on as arguments");
	}

	char buffer[1024];


	for(int i = 2; i < argc; ++i)
	{
		printf("%d\n", i);
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0)
		{
			error("Error opening socket");
		}
		struct sockaddr_in address;
		struct hostent* server;
		memset(&address, 0, sizeof(address));
		
		int port = atoi(argv[i]);
		server = gethostbyname(argv[1]);
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		
		memcpy((char*)&address.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
		

		int n = connect(fd, (struct sockaddr *)& address, sizeof(address));
		if (n < 0)
		{
			error("omg! cannot connect");
		}
	}	

	return 0;
}
