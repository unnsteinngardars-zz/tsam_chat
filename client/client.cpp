#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sstream>

struct hostent* server;
int BUFFER_LENGTH = 1024;

/**
 * Display an error and exit
*/
void error(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

/**
 * Create a socket FD
*/
int create_socket()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		error("Error opening socket");
	}
	return fd;
}

/**
 * Knock on a port
*/
void knock(int fd, sockaddr_in& address, int port)
{
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);		
	memcpy((char*)&address.sin_addr.s_addr, (char*) server->h_addr, server->h_length);

	int n = connect(fd, (struct sockaddr *)& address, sizeof(address));
	if (n < 0)
	{
		error("Error knocking");
	}
}


/**
 * Connect to the third port.
*/
void connect(int fd, sockaddr_in& address, int port)
{
	fd_set read_set, active_set;

	char recv_buffer[BUFFER_LENGTH];
	char send_buffer[BUFFER_LENGTH];
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);		
	memcpy((char*)&address.sin_addr.s_addr, (char*) server->h_addr, server->h_length);

	int n = connect(fd, (struct sockaddr *)& address, sizeof(address));
	if (n < 0)
	{
		error("Error connecting");
	}
	
	FD_ZERO(&active_set);
	FD_SET(fd, &active_set);
	/* add stdin to the active set */
	FD_SET(0, &active_set);

	while(1)
	{	
		read_set = active_set;
		if (select(fd + 1, &read_set, NULL, NULL, 0) < 0)
		{
			error("Failed to receive select socket");
		}
		
		for(int i = 0; i <= fd; ++i)
		{
			if (FD_ISSET(i, &read_set)){
				/* client */
				if (i == fd)
				{
					memset(recv_buffer, 0, BUFFER_LENGTH);
					int read_bytes = recv(fd, recv_buffer, BUFFER_LENGTH, 0);
					if (read_bytes < 0)
					{
						close(fd);
						error("Error reading from server");
					}
					else if (read_bytes == 0)
					{
						close(fd);
						return;
					}
					else {
						int buffer_length = strlen(recv_buffer);
						char local_buffer[buffer_length];
						memset(local_buffer, 0, buffer_length);
						memcpy(local_buffer, recv_buffer, buffer_length + 1);		
						printf("%s", local_buffer);

						/* some traces of us trying to move the cursor and then we wanted to store the cin buffer and restore it again */
						// printf("\033[0K");
						// printf("\r");
						// printf("%s", local_buffer);

					}
				}
				/* stdin */
				else if (i == 0)
				{
					memset(send_buffer, 0, BUFFER_LENGTH);
					fgets(send_buffer, BUFFER_LENGTH, stdin);
					int write_bytes = send(fd, send_buffer, BUFFER_LENGTH, 0);
					if (write_bytes < 0)
					{
						error("Error writing to server");
					}
				}
			}
		}

	}
}

int main(int argc, char *argv[])
{
	/* code */
	if (argc < 5)
	{
		error("invalid argument count, send in a host and three ports to knock on as arguments");
	}

	server = gethostbyname(argv[1]);

	struct sockaddr_in first_knock;
	struct sockaddr_in second_knock;
	struct sockaddr_in address;

	int fd_first_knock = create_socket();
	int fd_second_knock = create_socket();
	int fd_connect = create_socket();

	knock(fd_first_knock, first_knock, atoi(argv[2]));
	knock(fd_second_knock, second_knock, atoi(argv[3]));
	close(fd_first_knock);
	close(fd_second_knock);
	connect(fd_connect, address, atoi(argv[4]));
	close(fd_connect);
	return 0;
}
