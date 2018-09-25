#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>

/**
 * CHAT SERVER
 * 
 * much of the code in the main function is coded with inspiration from the following sources
 * http://beej.us/guide/bgnet/html/single/bgnet.html#getpeername
 * https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
*/

/* GLOBAL VARIABLES */
int server_port = 5923;
int MAX_CONNECTIONS_FOR_LISTEN_QUEUE = 10;
int MAX_BYTES = 512;
int MAX_USERNAME_SIZE = 15;
std::string welcome_message = "Welcome to LICKING LEMON LOLLIPOPS IN LILLEHAMMER chat server\n\nType HELP to display available commands\n\n";
std::string help_message = "List of options";

std::map<int, std::string> users_by_fd;
// std::map<char *, int> users_by_name;

int server_socket_fd;

int max_file_descriptor;

/* amount of bytes read from read/recv and write/send will be stored in this variable */
int read_bytes, write_bytes;

/* file descriptor sets, active holds all currently connected fds along with the listenin fd */
fd_set active_set, read_set;

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
 * @param socketfd the file descriptor to listen on
*/
void listen(int socketfd)
{
	/* Attempt to listen */
	if (listen(socketfd, MAX_CONNECTIONS_FOR_LISTEN_QUEUE) < 0)
	{
		error("Failed to listen");
	}
}

/**
 * Gets the username assigned to the corresponding file descriptor
 * If no username is found, anonymous is returned
 * @param fd the file desctiptor to get user by
*/
const char *getUserNameByFd(int fd)
{
	const char *username;
	try
	{
		username = users_by_fd.at(fd).c_str();
	}
	catch (const std::out_of_range &e)
	{
		username = (char *)"anonymous";
	}
	return username;
}

/**
 * Returns a allocated send buffer from body prefixed with username/anonymous
 * Remember to free afterwards!
 * @param username the username to prefix
 * @param body the message
 * @param length the length of the buffer
*/
char *createSendBuffer(const char *username, char *body, int &length)
{
	char *seperator = (char *)": ";
	length = strlen(username) + strlen(seperator) + strlen(body);
	char *buffer = (char *)malloc(sizeof(char) * length);
	memset(buffer, 0, length);
	memcpy(buffer, username, strlen(username));
	memcpy(buffer + strlen(username), seperator, strlen(seperator));
	memcpy(buffer + strlen(username) + strlen(seperator), body, strlen(body));
	return buffer;
}

/**
 * Remove leading and trailing white space from string
 * Borrowed from http://www.cplusplus.com/faq/sequences/strings/trim/
*/
inline std::string trim_right_copy(
	const std::string &s,
	const std::string &delimiters = " \f\n\r\t\v")
{
	return s.substr(0, s.find_last_not_of(delimiters) + 1);
}

/**
 * Get file descriptor by username
 * @param username the username
*/
int getFdByUserName(char *username)
{
	int fd = -1;
	std::string username_to_find = std::string(trim_right_copy(username));
	std::map<int, std::string>::iterator it;
	for (it = users_by_fd.begin(); it != users_by_fd.end(); ++it)
	{
		if (!it->second.compare(username_to_find))
		{
			fd = it->first;
		}
	}
	return fd;
}

/**
 * Send a message body to all users
 * @param body the message
 * @param current_fd the file descriptor of the sender
*/
void sendToAll(char *body, int current_fd)
{
	const char *username = getUserNameByFd(current_fd);
	int length = 0;
	char *buffer = createSendBuffer(username, body, length);

	for (int j = 0; j <= max_file_descriptor; j++)
	{
		/* Check if the client is in the active set */
		if (FD_ISSET(j, &active_set))
		{
			/* prevent the received message from the client from beeing sent to the server and himself */
			if (j != server_socket_fd && j != current_fd)
			{
				write_bytes = write(j, buffer, length);
				if (write_bytes < 0)
				{
					error("Sending to clients");
				}
			}
		}
	}
	free(buffer);
}

/**
 * Send a message body to a single user
 * @param body the message
 * @param current_fd the file descriptor of the sender
 * @param recv_fd the file descriptor of the recipient
*/
void sendToUser(char *body, int current_fd, int recv_fd)
{
	const char *username = getUserNameByFd(current_fd);
	int length = 0;
	char *buffer = createSendBuffer(username, body, length);

	if (FD_ISSET(recv_fd, &active_set))
	{
		write_bytes = write(recv_fd, buffer, length);
		if (write_bytes < 0)
		{
			error("Sending to client");
		}
	}
	free(buffer);
}

/**
 * Add a user to the map
*/
void addUser(int fd, char *username)
{
	if (users_by_fd.count(fd) == 1)
	{
	}
	std::string username_to_add = std::string(trim_right_copy(username));
	users_by_fd.insert(std::pair<int, std::string>(fd, username_to_add));
}

/**
 * Display a list of all connected users with usernames
*/
void displayUsers(int current_fd)
{
	std::string users = "\nLIST OF USERS\n";

	std::map<int, std::string>::iterator it;
	for (it = users_by_fd.begin(); it != users_by_fd.end(); ++it)
	{
		users += it->second + "\n";
	}
	users += "\n";

	int length = strlen(users.c_str());
	char *buffer = (char *)malloc(sizeof(char) * length);
	memset(buffer, 0, length);
	memcpy(buffer, users.c_str(), length);
	write_bytes = write(current_fd, buffer, length);
	if (write_bytes < 0)
	{
		error("Sending user list to self");
	}
}

void displayCommands(int current_fd)
{
	// TODO:: ADD ID commands!
	std::string help_message = "\nAvailable commands are:\n\n";
	help_message += "CONNECT <username>\tIdentify yourself with username\n";
	help_message += "LEAVE\t\t\tLeave chatroom\n";
	help_message += "WHO\t\t\tGet list of connected users\n";
	help_message += "MSG <user> <message>\tSend a message to specific user\n";
	help_message += "MSG ALL <message>\tSend message to all users connected\n";
	help_message += "HELP\t\t\tSe available commands\n\n";
	write(current_fd, help_message.c_str(), help_message.length());
}

/**
 * Parse the client input
 * @param buffer the buffer from the client
 * @param fd the clients file desctiptor
*/
void parseInputFromClient(char *buffer, int fd)
{
	char local_buffer[read_bytes];
	memset(local_buffer, 0, read_bytes);
	strncpy(local_buffer, buffer, read_bytes + 1);
	/* extract the first word in the string as the command */

	const char *command = strtok(local_buffer, " ");

	/* extract the second word in the string as sub_command/body */
	char *sub_command = strtok(NULL, " ");

	/* trim leading/trailing whitespace from command */
	command = trim_right_copy(std::string(command)).c_str();
	// sub_command = trim_right_copy(std::string(sub_command)).c_str();

	if (command == ID)
	{
		// TODO :: implement this fucker!
	}
	else if (command == CONNECT)
	{
		// SET USERNAME
		if (sub_command != NULL)
		{
			printf("client with fd %d connecting with username %s\n", fd, sub_command);
			if (strlen(trim_right_copy(std::string(sub_command)).c_str()) > MAX_USERNAME_SIZE)
			{
				std::string username_length_exceeded = "Username must be less than " + std::to_string(MAX_USERNAME_SIZE) + "\n";
				write(fd, username_length_exceeded.c_str(), username_length_exceeded.length());
			}
			else
			{
				addUser(fd, sub_command);
			}
		}
	}
	else if (command == LEAVE)
	{
		/* print info for server */
		std::string user_leaving = getUserNameByFd(fd);
		printf("user %s has left the server\n", user_leaving.c_str());

		/* inform other users I have left */
		char *body = (char *)"I have left the chat for now, goodbye :)\n";
		sendToAll(body, fd);

		/* clean up my shit */
		users_by_fd.erase(fd);
		FD_CLR(fd, &active_set);
		close(fd);
	}
	else if (command == WHO)
	{
		displayUsers(fd);
	}
	else if (command == MSG)
	{
		char *body = strtok(NULL, "");
		const char *sending_user = getUserNameByFd(fd);
		if (sub_command == ALL)
		{
			/* print info for server */
			printf("user %s sending message to everyone\n", sending_user);

			/* send message to everyone */
			sendToAll(body, fd);
		}
		else
		{

			int recipient = getFdByUserName(sub_command);

			/* print info for the server */
			printf("user %s sending message to %s\n", sending_user, getUserNameByFd(recipient));

			/* Send message to user if exists */
			if (recipient > 0)
			{
				sendToUser(body, fd, recipient);
			}
			else
			{
				/* inform the asking client that there is no such user  */
				std::string no_user_found_message = "No user found with username " + std::string(sub_command) + "\n";
				write(fd, no_user_found_message.c_str(), no_user_found_message.length());
			}
			// else write feedback
		}
	}
	else if (command == CHANGE)
	{
		if (sub_command == ID)
		{
			// CHANGE ID
		}
	}
	else if (command == HELP)
	{
		// DISPLAY HELP
		displayCommands(fd);
	}
	else
	{
		// INVALID COMMAND
		std::string invalid_command = "Invalid command\nType HELP for list of available commands\n";
		write(fd, invalid_command.c_str(), invalid_command.length());
	}
}

int main(int argc, char const *argv[])
{
	/* Variable declarations */
	int new_server_socket_fd, server_port;

	/* server port */
	/**
	 * TODO: Implement port knocking
	 * Search for three consecutive ports and pick for port knocking
	 * server will be on skel and hardcoded port might be used by another server
	*/
	server_port = 5923;

	/* the struct for the incomming client info */
	struct sockaddr_in client;

	/* client_length is used for the accept call
	 * http://pubs.opengroup.org/onlinepubs/009695399/functions/accept.html
	 * Points to a socklen_t structure which on input specifies the length of the supplied sockaddr structure, and on output specifies the length of the stored address.
	*/
	socklen_t client_length;

	/* buffer for messages */
	char buffer[MAX_BYTES];

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

					printf("Connection established from host: %s, port: %d with fd: %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port), new_server_socket_fd);

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
					memset(buffer, 0, MAX_BYTES);
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
						/* parse the client input */
						printf("bytes received: %d\n", read_bytes);
						parseInputFromClient(buffer, i);
					}
				}
			}
		}
	}

	return 0;
}