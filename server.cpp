#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* GLOBAL VARIABLES */
int server_port = 5923;
int MAX_CONNECTIONS_FOR_LISTEN_QUEUE = 10;
int MAX_BYTES = 256;
std::string welcome_message = "Welcome to LICKING LEMON LOLLIPOPS IN LILLEHAMMER chat server\n\nType HELP to display available commands\n\n";
std::map<int, std::string> users;

/* COMMANDS */
const std::string ID = "ID";
const std::string CONNECT = "CONNECT";
const std::string LEAVE = "LEAVE";
const std::string WHO = "WHO";
const std::string MSG = "MSG";
const std::string CHANGE = "CHANGE";
const std::string ALL = "ALL";
const std::string HELP = "HELP";

/**
 * Custom error function
*/
void error(const char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

/**
 * Create and Bind the socket
 * Create a socket file descriptor
 * Bind the socket to the local IP and a port passed as argument and set options
 * @param port the server port number to use
 * @return the created socket file descriptor
*/
int createAndBindSocket(int port)
{
	/* Variable declarations */
	int server_socket_fd;
	struct sockaddr_in server;

	/* memset with zeroes to populate sin_zero with zeroes */
	memset(&server, 0, sizeof(server));

	/* Create the socket file descriptor */
	server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);

	/* check for error */
	if (server_socket_fd < 0)
	{
		error("Failed creating socket");
	}

	/* Set address information */
	server.sin_family = AF_INET;
	// Use INADDR_ANY to bind to the local IP address
	server.sin_addr.s_addr = INADDR_ANY;
	// htons (host to network short) used to convert the port from host byte order to network byte order
	server.sin_port = htons(port);

	/* Prevent the Address aldready in use message when the socket still hangs around in the kernel after server shutting down */
	int the_integer_called_one = 1;
	if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &the_integer_called_one, sizeof(the_integer_called_one)) < 0)
	{
		error("Failed to prevent \"Address aldready in use\" message");
	}

	/* Associate socket with server IP and PORT */
	if (bind(server_socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		error("Failed to bind to socket");
	}

	return server_socket_fd;
}

/**
 * Wrapper function for the listen system call
*/
void listen(int socketfd)
{
	/* Attempt to listen */
	if (listen(socketfd, MAX_CONNECTIONS_FOR_LISTEN_QUEUE) < 0)
	{
		error("Failed to listen");
	}
}

void parseCommand(char *buffer, int fd)
{
	char *command = strtok(buffer, " ");
	char *nextCommand = strtok(NULL, " ");

	printf("%s\n", command);

	if (command == ID)
	{
		// UNAUTHORIZED UNLESS SERVER
	}
	else if (command == CONNECT)
	{
		// SET USERNAME
		users.insert(std::pair<int, std::string>(fd, nextCommand));
	}
	else if (command == LEAVE)
	{
		// LEAVE CHAT :(
	}
	else if (command == WHO)
	{
		// SEND LIST OF USERS
	}
	else if (command == MSG)
	{
		if (nextCommand == ALL)
		{
			// SEND TO ALL
		}
		else
		{
			// SEND TO NEXTCOMMAND USER
		}
	}
	else if (command == CHANGE)
	{
		if (nextCommand == ID)
		{
			// CHANGE ID
		}
	}
	else if (command == HELP)
	{
		// DISPLAY HELP
	}
	else
	{
		// INVALID COMMAND
	}
}

int main(int argc, char const *argv[])
{
	/* Variable declarations */
	int server_socket_fd, new_server_socket_fd, server_port, max_file_descriptor;

	/* server port */
	/**
	 * TODO: Implement port knocking
	 * Search for three consecutive ports and pick for port knocking
	 * server will be on skel and hardcoded port might be used by another server
	*/
	server_port = 5923;

	/* the struct for the incomming client info */
	struct sockaddr_in client;

	/* file descriptor sets, active holds all currently connected fds along with the listenin fd */
	fd_set active_set, read_set;

	/* client_length is used for the accept call
	 * http://pubs.opengroup.org/onlinepubs/009695399/functions/accept.html
	 * Points to a socklen_t structure which on input specifies the length of the supplied sockaddr structure, and on output specifies the length of the stored address.
	*/
	socklen_t client_length;

	/* buffer for messages */
	char buffer[MAX_BYTES];

	/* amount of bytes read from read/recv and write/send will be stored in this variable */
	int read_bytes, write_bytes;

	/* Create the socket for the server */
	server_socket_fd = createAndBindSocket(server_port);
	listen(server_socket_fd);

	/* Zero the whole active set */
	FD_ZERO(&active_set);
	/* Set the server file descriptor in the active set */
	FD_SET(server_socket_fd, &active_set);

	max_file_descriptor = server_socket_fd;

	while (1)
	{
		/* copy active_set to read_set to not loose information about active_set status since select alters the set passed as argument */
		read_set = active_set;
		/* wait for incomming client/s */
		if (select(max_file_descriptor + 1, &read_set, NULL, NULL, NULL) < 0)
		{
			error("Failed to receive client connection");
		}

		for (int i = 0; i <= max_file_descriptor; i++)
		{
			if (FD_ISSET(i, &read_set))
			{
				/* if i is our server_socket_fd, then it means it is ready to receive an incomming connection */
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

					write(new_server_socket_fd, welcome_message.c_str(), welcome_message.length());

					/* Add new connection to active set */
					FD_SET(new_server_socket_fd, &active_set);

					/* Set max file descriptor */
					if (new_server_socket_fd > max_file_descriptor)
					{
						max_file_descriptor = new_server_socket_fd;
					}
				}
				/* i is some client already connected */
				else
				{

					/* Already established connection */
					memset(buffer, 0, sizeof(buffer));
					read_bytes = read(i, buffer, MAX_BYTES);

					if (read_bytes < 0)
					{
						error("Error reading from client");
					}

					/* if read returns 0, the client has closed the connection
					 * so we need to close the clients socket file descriptor 
					 * and remove it from the active set 
					*/
					else if (read_bytes == 0)
					{
						close(i);
						FD_CLR(i, &active_set);
					}
					else
					{
						// tjekka buffer og acta a rett command
						parseCommand(buffer, i);

						printf("Message: %s\n", buffer);

						/* MSG ALL */
						/* Loop through all connected clients */
						for (int j = 0; j <= max_file_descriptor; j++)
						{
							/* Check if the client is in the active set */
							if (FD_ISSET(j, &active_set))
							{
								/* prevent the received message from the client from beeing sent to the server and himself */
								if (j != server_socket_fd && j != i)
								{
									write_bytes = write(j, buffer, read_bytes);
									if (write_bytes < 0)
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
