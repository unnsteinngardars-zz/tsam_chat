#include "commands.h"

class Server
{
  public:
	Server(){};
};

int main(int argc, char const *argv[])
{
	Server server;
	printf("help: %s\n", HELP.c_str());
	return 0;
}
