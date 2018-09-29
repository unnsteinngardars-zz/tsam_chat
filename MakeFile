CC =gcc
CXX =g++
CXXFLAGS =-g -Wall --std=c++11

# SERVER = entry.cpp server2.cpp socket_utilities.cpp string_utilities.cpp buffer_content.cpp
SERVER = $(wildcard ./server/*.cpp)
UTILITIES = $(wildcard ./utilities/*.cpp)
CLIENT = $(wildcard ./client/*.cpp)

server: $(SERVER) $(UTILITIES) 
	$(CXX) $(CXXFLAGS) -o chat_server $(SERVER) $(UTILITIES)
client: $(CLIENT)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT) 