#ifndef SERVER2_H
#define SERVER2_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "socket_utilities.h"
#include "string_utilities.h"
#include "buffer_content.h"

typedef std::vector<std::pair<int, struct sockaddr_in> > Servers;
typedef std::pair<int, struct sockaddr_in> Pair;


class Server
{
	private:
	Servers servers;
	// std::map<int, int> port_knocks;
	time_t knock_start, knock_stop;
	int MAX_BUFFER_SIZE;
	fd_set active_set, read_set; // might want to add write_set
	int max_file_descriptor;

  public:
	Server();
	int run();
	void set_timer();
	void stop_timer();
	int get_time_in_seconds();
	void set_max_buffer(int size);
	void create_sockets();
	void add_to_active_set();
	void listen_to_sockets();
	void rebind(Pair& pair);
	void update_max_fd(int fd);
	int accept_connection(int socket, sockaddr_in& address, socklen_t & length);
	BufferContent parse_buffer(char * buffer, int fd);
};

#endif