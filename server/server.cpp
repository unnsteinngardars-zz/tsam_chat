#include "server.h"

Server::Server()
{
	FD_ZERO(&active_set);
	set_fortune();
}


void Server::set_fortune()
{
	FILE* fp;
	char path[1035];
	std::string fort = "";
	fp = popen("/bin/fortune", "r");
	if (fp == NULL){
		printf("No command f");
	}
	while (fgets(path, sizeof(path) - 1, fp) != NULL){
		fort += std::string(path);
	}
	pclose(fp);
	if (fort.compare(""))
	{
		fort += "_GROUP_INITIALS_" + time_utilities::get_time_stamp();
		id = fort;
	}
	else{
		id = "NO_ID";
	}
	printf("fortune: %s\n", id.c_str());
}

std::string Server::get_fortune()
{
	return id;
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
	content_buffer.set_file_descriptor(fd);

	int buffer_length = strlen(buffer);
	char local_buffer[buffer_length];
	memset(local_buffer, 0, buffer_length);
	strncpy(local_buffer, buffer, buffer_length + 1);
	// printf("buffer_length: %d\n", strlen(local_buffer));

	char * first = strtok(local_buffer, " ");
	char * second = strtok(NULL, " ");
	char * third = strtok(NULL, " ");
	
	/* work with first string of buffer */
	string_utilities::trim_cstr(first);
	std::string command = (strlen(first) > 0) ? std::string(first) : "";
	content_buffer.set_command(command);
	
	/* work with second string of buffer */
	if (second != NULL)
	{
		string_utilities::trim_cstr(second);
		content_buffer.set_sub_command(std::string(second));
	}

	/* work with third string of buffer */
	if (third != NULL){
		string_utilities::trim_cstr(third);
		content_buffer.set_body(std::string(third));
	}

	// printf("command: %s\n", content_buffer.get_command().c_str());
	// printf("sub_command: %s\n", content_buffer.get_sub_command().c_str());
	// printf("body: %s\n", content_buffer.get_body().c_str());

	return content_buffer;
}


void Server::display_commands(int fd)
{
	// TODO:: ADD ID commands!
	std::string help_message = "\nAvailable commands are:\n\n";
	help_message += "ID\t\tGet the ID of the server\n";
	help_message += "CHANGE ID\t\tChange the ID of the server\n";
	help_message += "CONNECT <username>\tIdentify yourself with username, cannot include space\n";
	help_message += "LEAVE\t\t\tLeave chatroom\n";
	help_message += "WHO\t\t\tGet list of connected users\n";
	help_message += "MSG <user> <message>\tSend a message to specific user\n";
	help_message += "MSG ALL <message>\tSend message to all users connected\n";
	help_message += "HELP\t\t\tSe available commands\n\n";
	write(fd, help_message.c_str(), help_message.length());
}

void Server::display_users(BufferContent& content_buffer)
{
	int fd = content_buffer.get_file_descriptor();
	std::string users = "\nLIST OF USERS:\n";
	std::set<std::string>::iterator it;
	for (it = usernames_set.begin(); it != usernames_set.end(); ++it){
		std::string username = *it;
		users += " " + username + "\n";
	}
	socket_utilities::write_to_client(fd, users);
}


bool Server::add_user(BufferContent& buffer_content, std::string& feedback_message)
{
	std::string username = buffer_content.get_sub_command() + buffer_content.get_body();
	int fd = buffer_content.get_file_descriptor();
	if (username.size() > 15 || !username.compare(""))
	{	
		feedback_message = "Username cannot be more than 15 characters\n";
		return false;
	}
	else if (usernames.count(fd) > 0)
	{
		feedback_message = "You have already connected with a username\n";
		return false;
	}
	else if (usernames_set.count(username) > 0)
	{
		feedback_message = "Username already taken\n";
		return false;
	}
	usernames.insert(std::pair<int, std::string>(fd, username));
	usernames_set.insert(username);
	feedback_message = username + " has logged in to the server\n";
	return true;
}

void Server::send_to_all(BufferContent& buffer_content)
{	
	int fd = buffer_content.get_file_descriptor();
	std::string body = buffer_content.get_body();
	for (int i = 0; i <= max_file_descriptor; i++)
	{
		/* Check if the client is in the active set */
		if (FD_ISSET(i, &active_set))
		{
			/* prevent the received message from the client from beeing sent to the server and himself */
			if (i != servers.at(0).first && i != servers.at(1).first && i != servers.at(2).first && i != fd)
			{
				int write_bytes = socket_utilities::write_to_client(i, body);
			}
		}
	}
}

void Server::send_to_user(int rec_fd, BufferContent& content_buffer)
{
	std::string body = content_buffer.get_body();
	if (FD_ISSET(rec_fd, &active_set)){
		int write_bytes = socket_utilities::write_to_client(rec_fd, body);
	}
}

void Server::remove_from_set(std::string username)
{
	std::set<std::string>::iterator it;
	for (it = usernames_set.begin(); it != usernames_set.end(); ++it)
	{	
		std::string user = *it;
		if (!user.compare(username)){
			usernames_set.erase(it);
			break;
		}
	}
}

bool Server::user_exists(int fd)
{
	if (usernames.count(fd) == 1) 
	{
		return true;
	}
	return false;
}

int Server::get_fd_by_user(std::string username)
{
	int fd = -1;
	std::map<int, std::string>::iterator it;
	for (it = usernames.begin(); it != usernames.end(); ++it)
	{
		std::string user = it->second;
		if (!user.compare(username))
		{		
			fd = it->first;
		}
	}
	return fd;
}

void Server::execute_command(BufferContent& content_buffer)
{	
	std::string command = content_buffer.get_command();
	std::string sub_command;
	std::string feedback_message;
	int fd = content_buffer.get_file_descriptor();

	if ((!command.compare("ID"))) 
	{
		printf("display server id\n");

		socket_utilities::write_to_client(fd, id);
	}

	else if ((!command.compare("CONNECT"))) 
	{	
		if (add_user(content_buffer, feedback_message)){
			// write to all
			content_buffer.set_body(feedback_message);
			send_to_all(content_buffer);
		}
		else{
			write(fd, feedback_message.c_str(), feedback_message.length());
		}

	}
	else if ((!command.compare("LEAVE"))) 
	{
		
		if (user_exists(fd)){
			std::string username = usernames.at(fd);
			content_buffer.set_body(username + " has left the chat\n");
			send_to_all(content_buffer);
			usernames.erase(fd);
			remove_from_set(username);

		}
		socket_utilities::close_socket(fd);
		FD_CLR(fd, &active_set);


	}
	else if ((!command.compare("WHO"))) 
	{
		display_users(content_buffer);

	}
	else if ((!command.compare("MSG"))) 
	{
		sub_command = content_buffer.get_sub_command();
		if (user_exists(fd)){
			// get sending user
			std::string sending_user = usernames.at(fd);
			if (!sub_command.compare("ALL"))
			{
				// send to all
				printf("send to all\n");
				content_buffer.set_body(sending_user + ": " + content_buffer.get_body() + "\n");
				send_to_all(content_buffer);
			}
			else
			{
				// send to single user
				int rec_fd = get_fd_by_user(sub_command);
				if (rec_fd > 0)
				{	
					// do not send to yourself
					if(fd != rec_fd)
					{	
						content_buffer.set_body(sending_user + ": " + content_buffer.get_body() + "\n");
						send_to_user(rec_fd, content_buffer);
					}
					else
					{
						feedback_message = "Cannot send message to yourself\n";
						socket_utilities::write_to_client(fd, feedback_message);
						// write(fd,feedback_message.c_str(), feedback_message.length());
					}
				}
				// no user found
				else
				{	
					printf("no user found\n");
					feedback_message = "No such user\n";
					socket_utilities::write_to_client(fd, feedback_message);
					// write(fd, feedback_message.c_str(), feedback_message.length());
				}
			}
		}

		else 
		{
			feedback_message = "You need to be logged in\n";
			write(fd, feedback_message.c_str(), feedback_message.length());
		}
	
	}
	else if ((!command.compare("CHANGE"))) 
	{
		sub_command = content_buffer.get_sub_command();
		if( (!sub_command.compare("ID")) )
		{
			printf("change id\n");
		}
	}
	else if ( (!command.compare("HELP")) )
	{
		display_commands(content_buffer.get_file_descriptor());
	}

	else
	{
		feedback_message = "Unknown command, type HELP for commands\n";
		socket_utilities::write_to_client(fd, feedback_message);
	}

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
						printf("Connection established from %s port %d and fd %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port), client_fd);
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
						BufferContent content_buffer;
						std::string feedback_message;
						content_buffer.set_file_descriptor(i);
						
						if (user_exists(i)){
							std::string username = usernames.at(i);
							content_buffer.set_body(username + " has left the chat");
							send_to_all(content_buffer);
							usernames.erase(i);
							remove_from_set(username);
						}
						socket_utilities::close_socket(i);
						FD_CLR(i, &active_set);
					}
					else {
						// printf("Buffer from client: %s\n", buffer);
						// printf("buffer length: %zu\n", strlen(buffer));
						BufferContent buffer_content = parse_buffer(buffer, i);
						execute_command(buffer_content);
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
