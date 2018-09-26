#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <time.h>

/**
 * CHAT SERVER
 * 
 * much of the code in the main function is coded with inspiration from the following sources
 * http://beej.us/guide/bgnet/html/single/bgnet.html#getpeername
 * https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
*/

/* GLOBAL VARIABLES */

int MIN_PORT = 1024;
int MAX_PORT = 65532;

int first_port, second_port, third_port;

int MAX_CONNECTIONS_FOR_LISTEN_QUEUE = 10;
int MAX_BYTES = 512;
int MAX_USERNAME_SIZE = 15; //update feedback string if this value is changed

std::map<int, std::string> users_by_fd;
std::set<std::string> usernames;

struct sockaddr_in server;
struct sockaddr_in server_second;
struct sockaddr_in server_third;

int server_socket_fd;
int server_socket_fd_second;
int server_socket_fd_third;

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
int create_socket()
{
	/* Variable declarations */
	int server_socket_fd;

	/* Create the socket file descriptor */
	server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);

	/* check for error */
	if (server_socket_fd < 0)
	{
		error("Failed creating socket");
	}
	/* Prevent the Address aldready in use message when the socket still hangs around in the kernel after server shutting down */
	int the_integer_called_one = 1;
	if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &the_integer_called_one, sizeof(the_integer_called_one)) < 0)
	{
		error("Failed to prevent \"Address aldready in use\" message");
	}

	return server_socket_fd;
}

void find_and_bind()
{
	bool found = false;

	/* memset with zeroes to populate sin_zero with zeroes */
	memset(&server, 0, sizeof(server));
	memset(&server_second, 0, sizeof(server_second));
	memset(&server_third, 0, sizeof(server_third));
	/* Set address information */
	server.sin_family = AF_INET;
	server_second.sin_family = AF_INET;
	server_third.sin_family = AF_INET;
	// Use INADDR_ANY to bind to the local IP address
	server.sin_addr.s_addr = INADDR_ANY;
	server_second.sin_addr.s_addr = INADDR_ANY;
	server_third.sin_addr.s_addr = INADDR_ANY;

	/* Associate sockets with server IP and PORTS */
	while (!found)
	{
		for (first_port = MIN_PORT; first_port < MAX_PORT; ++first_port)
		{
			second_port = first_port + 1;
			third_port = first_port + 2;
			// htons (host to network short) used to convert the port from host byte order to network byte order
			server.sin_port = htons(first_port);
			server_second.sin_port = htons(second_port);
			server_third.sin_port = htons(third_port);

			if (bind(server_socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
			{
				continue;
			}
			else
			{
				if (bind(server_socket_fd_second, (struct sockaddr *)&server_second, sizeof(server_second)) < 0)
				{
					continue;
				}
				else
				{
					if (bind(server_socket_fd_third, (struct sockaddr *)&server_third, sizeof(server_third)) < 0)
					{
						continue;
					}
					else
					{
						found = true;
					}
				}
			}
		}
		if (!found)
		{
			error("Failed to bind to sockets");
		}
	}
	printf("Server listening to incomming sockets on ports %d, %d and %d\n", first_port, second_port, third_port);
	printf("Knocking sequence is %d %d %d\n", third_port, second_port, first_port);
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

void rebind_and_listen(int i)
{
	struct sockaddr_in temp;
	memset(&temp, 0, sizeof(temp));

	temp.sin_family = AF_INET;
	temp.sin_addr.s_addr = INADDR_ANY;
	temp.sin_port = htons(1025);

	if (bind(i, (struct sockaddr *)&temp, sizeof(temp)) < 0)
	{
		error("failed to rebind socket");
	}

	if (listen(i, MAX_CONNECTIONS_FOR_LISTEN_QUEUE) < 0)
	{
		error("Failed to listen");
	}
	FD_SET(i, &active_set);
}

/**
 * Gets the username assigned to the corresponding file descriptor
 * If no username is found, anonymous is returned
 * @param fd the file desctiptor to get user by
*/
const char *get_user_by_fd(int fd)
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
char *create_data_buffer(const char *username, char *body, int &length)
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
inline std::string trim_string(const std::string &s, const std::string &delimiters = " \f\n\r\t\v")
{
	return s.substr(0, s.find_last_not_of(delimiters) + 1);
}

/**
 * Get file descriptor by username
 * @param username the username
 * @returns the file descriptor if found, else -1
*/
int get_fd_by_user(char *username)
{
	int fd = -1;
	std::string username_to_find = std::string(trim_string(username));
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
void send_to_all(char *body, int current_fd)
{
	const char *username = get_user_by_fd(current_fd);
	int length = 0;
	char *buffer = create_data_buffer(username, body, length);

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
void send_to_user(char *body, int current_fd, int recv_fd)
{
	if (users_by_fd.count(current_fd) == 1)
	{
		const char *username = get_user_by_fd(current_fd);
		int length = 0;
		char *buffer = create_data_buffer(username, body, length);

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
	else
	{
		std::string no_username = "You must use register with a username before sending private messages\n";
		write(current_fd, no_username.c_str(), no_username.length());
	}
}

/**
 * Add a user to the map
*/
void add_user(int fd, char *username, char *body)
{
	if ((strlen(username) > MAX_USERNAME_SIZE || strlen(username) < 1) || body != NULL)
	{
		std::string username_length_exceeded = "Username must be less than 15 characters\nUsername cannot contain white spaces\n";
		write(fd, username_length_exceeded.c_str(), username_length_exceeded.length());
	}
	else
	{
		std::string username_to_add = std::string(trim_string(username));
		// Check if username exists already;
		if (usernames.count(username_to_add) == 1)
		{
			std::string username_taken = "Username taken, please pick another one.\n";
			write(fd, username_taken.c_str(), username_taken.length());
		}
		else if (users_by_fd.count(fd) == 1)
		{
			std::string already_logged_in = "You have already picked a username\n";
			write(fd, already_logged_in.c_str(), already_logged_in.length());
		}
		else
		{
			std::string welcome_user = "You are know known as " + std::string(username) + " and other users can send you private messages\n";
			users_by_fd.insert(std::pair<int, std::string>(fd, username_to_add));
			usernames.insert(username_to_add);
			write(fd, welcome_user.c_str(), welcome_user.length());
		}
	}
}

/**
 * Display a list of all connected users with usernames
*/
void display_users(int current_fd)
{
	std::string users = "\nLIST OF UNANONYMOUS USERS\n";

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

/**
 * Display a list of commands
 * @param the fd of the user requesting the list
*/
void display_commands(int current_fd)
{
	// TODO:: ADD ID commands!
	std::string help_message = "\nAvailable commands are:\n\n";
	help_message += "CONNECT <username>\tIdentify yourself with username, cannot include space\n";
	help_message += "LEAVE\t\t\tLeave chatroom\n";
	help_message += "WHO\t\t\tGet list of connected users\n";
	help_message += "MSG <user> <message>\tSend a message to specific user\n";
	help_message += "MSG ALL <message>\tSend message to all users connected\n";
	help_message += "HELP\t\t\tSe available commands\n\n";
	write(current_fd, help_message.c_str(), help_message.length());
}

/**
 * Helper function to trim a c_str of leading/trailing whitespace
*/
void trim_cstr(char *cstr)
{
	if (cstr != NULL)
	{
		char temp[strlen(cstr)];
		memset(temp, 0, strlen(cstr));
		strncpy(temp, cstr, strlen(cstr) + 1);
		std::string str(cstr);
		std::string trimmed_str = trim_string(str);
		memset(cstr, 0, trimmed_str.length());
		strncpy(cstr, trimmed_str.c_str(), trimmed_str.length() + 1);
	}
	return;
}

void display_command_input(int fd)
{
	std::string input = "command: ";
	write(fd, input.c_str(), input.length());
}

/**
 * Friendly welcomming message to the client
*/
void welcome_client(int fd)
{
	std::string welcome_message = "--------------------------------------------------\n";
	welcome_message += "\t\t   WELCOME\n\n";
	welcome_message += "LICKING LEMON LOLLIPOPS IN LILLEHAMMER CHAT SERVER\n";
	welcome_message += "Type HELP to display available commands\n";
	welcome_message += "--------------------------------------------------\n\n";
	write(fd, welcome_message.c_str(), welcome_message.length());
}

/**
 * Parse the client input
 * @param buffer the buffer from the client
 * @param fd the clients file desctiptor
*/

void parse_client_input(char *buffer, int fd)
{
	/* Create a local buffer from received buffer */
	int length = strlen(buffer);
	char local_buffer[length];
	memset(local_buffer, 0, length);
	strncpy(local_buffer, buffer, length + 1);

	/* extract the first word in the string as the command */
	char *command = strtok(local_buffer, " ");
	trim_cstr(command);
	char *sub_command = strtok(NULL, " ");
	trim_cstr(sub_command);

	/* extract the second word in the string as sub_command/body */

	if (command == ID)
	{
		// TODO :: implement this fucker!
	}
	else if (command == CONNECT)
	{
		if (sub_command != NULL)
		{
			printf("client with socket file descriptor %d registering with username %s\n", fd, sub_command);
			char *body = strtok(NULL, " ");
			add_user(fd, sub_command, body);
			// display_command_input(fd);
		}
	}
	else if (command == LEAVE)
	{
		/* print info for server */
		std::string user_leaving = get_user_by_fd(fd);
		printf("user %s has left the server\n", user_leaving.c_str());

		/* inform other users I have left */
		char *body = (char *)"I have left the chat for now, goodbye :)\n";
		send_to_all(body, fd);

		/* clean up my shit */
		users_by_fd.erase(fd);
		FD_CLR(fd, &active_set);
		close(fd);
	}
	else if (command == WHO)
	{
		display_users(fd);
		// display_command_input(fd);
	}
	else if (command == MSG)
	{
		char *body = strtok(NULL, "");
		const char *sending_user = get_user_by_fd(fd);
		if (sub_command == ALL)
		{
			/* print info for server */
			printf("user %s sending message to everyone\n", sending_user);
			/* send message to everyone */
			send_to_all(body, fd);
		}
		else
		{
			int recipient = get_fd_by_user(sub_command);
			/* print info for the server */
			printf("user %s sending message to %s\n", sending_user, get_user_by_fd(recipient));
			/* Check if user exists */
			if (recipient > 0)
			{
				/* Send message to user */
				send_to_user(body, fd, recipient);
			}
			else
			{
				/* inform the asking client that there is no such user  */
				std::string no_user_found_message = "No user found with username " + std::string(sub_command) + "\n";
				write(fd, no_user_found_message.c_str(), no_user_found_message.length());
				// display_command_input(fd);
			}
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
		display_commands(fd);
		// display_command_input(fd);
	}
	else
	{
		std::string invalid_command = "Invalid command\nType HELP for list of available commands\n";
		write(fd, invalid_command.c_str(), invalid_command.length());
		// display_command_input(fd);
	}
}

int main(int argc, char const *argv[])
{

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
	server_socket_fd = create_socket();
	server_socket_fd_second = create_socket();
	server_socket_fd_third = create_socket();

	find_and_bind();

	listen(server_socket_fd);
	listen(server_socket_fd_second);
	listen(server_socket_fd_third);

	/* Zero the whole active set */
	FD_ZERO(&active_set);
	/* Set the server file descriptor in the active set */
	FD_SET(server_socket_fd, &active_set);
	FD_SET(server_socket_fd_second, &active_set);
	FD_SET(server_socket_fd_third, &active_set);

	printf("server fd 1: %d, 2: %d, 3: %d\n", server_socket_fd, server_socket_fd_second, server_socket_fd_third);

	max_file_descriptor = server_socket_fd_third;

	while (1)
	{
		/* copy active_set to read_set to not loose information about active_set status since select alters the set passed as argument */
		read_set = active_set;
		/* wait for incomming client/s */
		if (select(max_file_descriptor + 1, &read_set, NULL, NULL, 0) < 0)
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
					int new_server_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client, &client_length);
					if (new_server_socket_fd < 0)
					{
						error("Failed to establish connection");
					}

					printf("Connection established from host: %s port: %d with socket file descriptor: %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port), new_server_socket_fd);

					/* Welcomes user to the server */
					welcome_client(new_server_socket_fd);

					// display_command_input(new_server_socket_fd);
					/* Add new connection to active set */
					FD_SET(new_server_socket_fd, &active_set);

					/* Set max file descriptor */
					if (new_server_socket_fd > max_file_descriptor)
					{
						max_file_descriptor = new_server_socket_fd;
					}
				}
				else if (i == server_socket_fd_second)
				{

					close(i);
					FD_CLR(i, &active_set);
					i = create_socket();
					rebind_and_listen(i);
				}
				else if (i == server_socket_fd_third)
				{
					close(i);
					FD_CLR(i, &active_set);
					i = create_socket();
					rebind_and_listen(i);
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
						// printf("bytes received: %d\n", read_bytes);
						parse_client_input(buffer, i);
					}
				}
			}
		}
	}
	return 0;
}
