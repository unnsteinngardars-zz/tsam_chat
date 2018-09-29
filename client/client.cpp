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
	//
	if (argc < 4)
	{
		error("invalid argument count, send in three ports to knock on as arguments");
	}
	return 0;
}
