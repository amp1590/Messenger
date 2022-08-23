# messenger

CPDP_Project_2: A Simple Chat Application
**************

Author: Arunima Mandal
**************

Contents:
**************
There are 2 C++ Program Files along with some other necessary files.

Some of the significant files are: 
    chat_server.cpp
    chat_client.cpp
    server_config (input to the chat_server.cpp file)
    client_config (input to the chat_client.cpp file)
    Makefile
    README (this file)
All files are in the same directory.

Execution:
**************
To compile and get the executables run command: 
"make" [Makefile will be run in this single command. Running this command should produce two executable files - chat_server.x chat_client.]

Then run the following command:
"./chat_server.x server_config" (Server Initialization)
[User needs to update the port number in server_config file according to the execution environment]

Then run the following command:
"./chat_client.x client_config 25110" (Client Initialization)

[The user has to update the values for servhost and servport in the client_config file according to the execution environment]
[The user needs provide a port number other than the one specified in the files which is a different port no for each client]


**************

All the programs and files have been checked in linprog for execution.

**************

Command Options:
********************
If not loggedIn then:
1. Enter ' login username ' (Replace username with desired name) to login
2. Enter ' exit ' to quit the program

If LoggedIn then:
1. Enter ' chat @friend_username message ' to send message to a particular user. Messages with spaces can also be passed.
2. Enter ' chat message ' to send message to all. Messages with spaces can also be passed.
3. Enter ' logout ' to logout from the server.


Features:
****************
The necessary features available in this program is described below:
    1. After connecting to the client:
        a. A new user can login using the 'r' command. (No registration and password required! It is assumed that the users will be unique.)
        c. Client can quit the program using the 'exit' command.
        d. A logged in user can logout from the server using the 'logout' command.
        e. Any user can login from the client after being logged out [can be the old user or another user].

    2. Server side - I/O multiplexing : Done using select() function.
    3. Client side - I/O multiplexing : Done using POSIC() threads.
    4. Server and Client both programs can handle SIGINT signal. Opened connections are closed before exiting the program as well.
    5. STL containers (vectors, unordered_map, iterator etc) have been used as data structures on server and client side.
    6. All the messages for keep the communication consistent have been shown
   


Did not implemented any of the Bonus point questions.
************************

Acknowledgement and References:
*********************************

For implementing the program, I've taken the basic ideas from the books, the class lecture slides and sample programs used in the class. I have read several online blogs and github code for implementing Sockets and Threads. I used stackoverflow whenever I was stuck in somewhere during the implementation.

*********************************



