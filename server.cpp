#include <iostream>
#include <ostream>
#include <istream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void error(const char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

/**
 * 	
*/
int createServerSocket(int port)
{
	int server_socket_fd;
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));

	server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);

	if (server_socket_fd < 0)
	{
		error("Failed creating socket");
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	/* Assigning a name to the socket */
	if (bind(server_socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		error("Failed to bind to socket");
	}
	int option = 1;
	setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	/* Attempt to listen */
	if (listen(server_socket_fd, 5) < 0)
	{
		error("Failed to listen");
	}

	return server_socket_fd;
}

int main(int argc, char const *argv[])
{
	int server_socket_fd, new_server_socket_fd, server_port, max_file_descriptors;
	int n;

	server_port = 5923;

	struct sockaddr_in client;
	struct fd_set active_set, read_set;

	socklen_t client_length;
	int MAX_BYTES = 256;
	char buffer[MAX_BYTES];
	int read_bytes;

	/* Create the socket for the server */
	server_socket_fd = createServerSocket(server_port);

	FD_ZERO(&active_set);
	FD_SET(server_socket_fd, &active_set);

	max_file_descriptors = server_socket_fd;

	while (1)
	{
		read_set = active_set;
		/* wait for incomming clients */
		if (select(max_file_descriptors + 1, &read_set, NULL, NULL, NULL) < 0)
		{
			error("Failed to receive client connection");
		}

		for (int i = 0; i <= max_file_descriptors; i++)
		{
			if (FD_ISSET(i, &read_set))
			{
				if (i == server_socket_fd)
				{
					/* New connection */
					client_length = sizeof(client);
					new_server_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client, &client_length);
					if (new_server_socket_fd < 0)
					{
						error("Failed to establish connection");
					}

					printf("Connection established from host: %s, port: %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

					/* Add new connection to active set */
					FD_SET(new_server_socket_fd, &active_set);

					/* Set max file descriptor */
					if (new_server_socket_fd > max_file_descriptors)
					{
						max_file_descriptors = new_server_socket_fd;
					}
				}
				else
				{
					/* Already established connection */
					memset(buffer, 0, sizeof(buffer));
					read_bytes = read(i, buffer, MAX_BYTES);
					if (read_bytes < 0)
					{
						error("Error reading from client");
					}
					else if (read_bytes == 0)
					{
						close(i);
						FD_CLR(i, &active_set);
					}
					else
					{
						printf("Message: %s\n", buffer);

						/* Send to all connected clients*/
						for (int j = 0; j <= max_file_descriptors; j++)
						{
							if (FD_ISSET(j, &active_set))
							{
								/* prevent sending to the server and the sending client */
								if (j != server_socket_fd && j != i)
								{
									if (write(j, buffer, read_bytes) == -1)
									{
										error("Sending to clients");
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}
