#ifndef SOCKET_UTILITIES_H
#define SOCKET_UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>


#include <vector>

typedef std::vector<std::pair<int, struct sockaddr_in> > Servers;
typedef std::pair<int, struct sockaddr_in> Pair;

namespace socket_utilities
{

	void error(const char *message);

	int create_socket();

	int bind_to_address(int fd, struct sockaddr_in& address, int port);

	int find_consecutive_ports(int min_port, int max_port, Servers &servers);

	void listen_on_socket(int fd);

	void rebind_and_listen(int fd, sockaddr_in &address, int port);
	
	void close_socket(int fd);

	int write_to_client(int fd, std::string message);
} // namespace socket_util

#endif