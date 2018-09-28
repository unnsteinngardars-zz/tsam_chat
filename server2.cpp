#include "server2.h"

Server::Server()
{
	FD_ZERO(&active_set);
}

void Server::set_timer()
{
	time(&knock_start);
}

void Server::stop_timer()
{
	time(&knock_stop);
}

int Server::get_time_in_seconds()
{
	return difftime(knock_stop, knock_start);
}

void Server::set_max_buffer(int size)
{
	MAX_BUFFER_SIZE = size;
}

void Server::create_sockets()
{
	for (int i = 0; i < 3; ++i)
	{
		int socket = socket_utilities::create_socket();
		struct sockaddr_in address;
		servers.push_back(Pair(socket, address));
	}
}

void Server::add_to_active_set()
{
	for (int i = 0; i < servers.size(); ++i)
	{
		FD_SET(servers[i].first, &active_set);
	}
}

void Server::listen_to_sockets()
{
	for (int i = 0; i < servers.size(); ++i)
	{
		socket_utilities::listen_on_socket(servers[i].first);
	}
}

void Server::rebind(Pair& pair)
{
	int test = ntohs(pair.second.sin_port);
	int port = pair.second.sin_port;
	socket_utilities::close_socket(pair.first);
	FD_CLR(pair.first, &active_set);
	pair.first = socket_utilities::create_socket();
	socket_utilities::rebind_and_listen(pair.first, pair.second, port);
	FD_SET(pair.first, &active_set);
}

void Server::update_max_fd(int fd)
{
	if (fd > max_file_descriptor) {
		max_file_descriptor = fd;
	}
}

int Server::accept_connection(int socket, sockaddr_in& address, socklen_t & length)
{
	int new_socket = accept(socket, (struct sockaddr *)&address, &length);
	if(new_socket < 0){
		socket_utilities::error("Failed to establish connection");
	}
	return new_socket;
}


BufferContent Server::parse_buffer(char * buffer, int fd)
{
	BufferContent content_buffer;

	int buffer_length = strlen(buffer);
	char local_buffer[buffer_length];
	memset(local_buffer, 0, buffer_length);
	strncpy(local_buffer, buffer, buffer_length + 1);
	// printf("buffer_length: %d\n", strlen(local_buffer));

	char * first = strtok(local_buffer, " ");
	char * second = strtok(NULL, " ");
	char * third = strtok(NULL, " ");
	
	bool second_body = false;
	
	/* work with first part */
	string_utilities::trim_cstr(first);
	std::string command = (strlen(first) > 0) ? std::string(first) : "";
	content_buffer.set_command(command);
	
	if (second != NULL){
		string_utilities::trim_cstr(second);
		if ((strcmp(second, "ID") == 0) || (strcmp(second,"ALL") == 0)){
			content_buffer.set_sub_command(std::string(second));
		}
		else{
			content_buffer.set_body(std::string(second));
			second_body = true;
		}
	}

	if (third != NULL && !second_body){
		string_utilities::trim_cstr(third);
		content_buffer.set_body(std::string(third));
	}

	// printf("first: %s\n", first);
	// printf("second: %s\n", second);
	// printf("third: %s\n", third);

	printf("command: %s\n", content_buffer.get_command().c_str());
	printf("sub_command: %s\n", content_buffer.get_sub_command().c_str());
	printf("body: %s\n", content_buffer.get_body().c_str());



	return content_buffer;
}

int Server::run()
{
	int MIN_PORT = 1024;
	int MAX_PORT = 65532;
	/* the struct for the incomming client info */
	struct sockaddr_in client;

	/* client_length is used for the accept call
	 * http://pubs.opengroup.org/onlinepubs/009695399/functions/accept.html
	 * Points to a socklen_t structure which on input specifies the length of the supplied sockaddr structure, and on output specifies the length of the stored address.
	*/
	socklen_t client_length;

	/* buffer for messages */
	char buffer[MAX_BUFFER_SIZE];

	/* Create 3 sockets/sockaddr_in */
	create_sockets();

	/* find consecutive ports to bind to */
	if (socket_utilities::find_consecutive_ports(MIN_PORT, MAX_PORT, servers) < 0){
		socket_utilities::error("Failed to bind to ports");
	}

	/* listen and add to active set */
	listen_to_sockets();
	add_to_active_set();

	int first_port = ntohs(servers.at(0).second.sin_port);
	int second_port = ntohs(servers.at(1).second.sin_port);
	int third_port = ntohs(servers.at(2).second.sin_port);
	printf("ports %d %d %d\n", first_port, second_port, third_port);

	max_file_descriptor = servers.at(servers.size() - 1).first;
	printf("max: %d\n", max_file_descriptor);
	while (1)
	{
		/* copy active_set to read_set to not loose information about active_set status since select alters the set passed as argument */
		read_set = active_set;
		/* wait for incomming client/s */
		if (select(max_file_descriptor + 1, &read_set, NULL, NULL, 0) < 0)
		{
			socket_utilities::error("Failed to receive client connection");
		}

		for (int i = 0; i <= max_file_descriptor; i++)
		{
			if (FD_ISSET(i, &read_set))
			{
				/* if i is the first open port */
				if (i == servers.at(0).first)
				{
					printf("first knock\n");
					set_timer();
					rebind(servers.at(0));
					update_max_fd(i);
				}
				/* if i is the second open port */
				else if (i == servers.at(1).first)
				{
					printf("second knock\n");
					rebind(servers.at(1));
					update_max_fd(i);
				}
				/* if i is the third open port */
				else if (i == servers.at(2).first)
				{
					printf("third knock\n");
					stop_timer();
					if (get_time_in_seconds() < 4) {
						printf("Welcome bitch!\n");
						client_length = sizeof(client);
						int client_fd = accept_connection(i, client, client_length);
						printf("Connection established from %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
						FD_SET(client_fd, &active_set);
						update_max_fd(client_fd);

					}
					else {
						printf("to late!\n");
						rebind(servers.at(2));
						update_max_fd(i);
					}

				}
				/* i is some already connected client that has send a message */
				else {
					memset(buffer, 0, MAX_BUFFER_SIZE);
					int read_bytes = read(i, buffer, MAX_BUFFER_SIZE);
					if (read_bytes < 0){
						socket_utilities::error("Error reading from client");
					}
					else if (read_bytes == 0){
						socket_utilities::close_socket(i);
						FD_CLR(i, &active_set);
					}
					else {
						printf("Buffer from client: %s\n", buffer);
						printf("buffer length: %zu\n", strlen(buffer));
						BufferContent buffer_content = parse_buffer(buffer, i);
					}
				}

			}
			else
			{
				
			}
		}
	}
	return 0;
}
