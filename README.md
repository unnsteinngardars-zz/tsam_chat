# tsam_chat

This project is a school assignment from Reykjavík University. 
The reason for this repository being public is to showcase code examples. 
Copying this code to use for the same assignment is considered plagiarism 
and will likely result in a penalty carried out by the teacher.

## Compile and Run
compile the server with the following command
```make server```
compile the client with the following command
```make client```

## Environment
The client and server were developed on macOS High Sierra version 10.13.4 and frequently set up and run on skel also

## Code
The source code is split up into three directories, the server, client and utilities
Though the server mainly uses the utilities we thought it would make for a cleaner code.

## External resources
We used Brian Halls web tutorial alot for inspiration
[Beej's guide to network programming](http://beej.us/guide/bgnet/html/multi/index.html)

## Additional information
We really wanted to find a solution to the issue that when a client is in the middle of typing something and another client sends a message or executes a command which 
writes to the client in the middle of his typing. The result is appended to the text on the screen.
User 1 is typing CONNECT username but has not hit enter, User 2 then types CONNECT second_user and hits enter. User 1 will then "see CONNECT usernamesecond_user has connected" on his screen.
But when he hits enter he only executes the CONNECT username command.

Simply due to time we could not attend to this issue but are full aware of it.


