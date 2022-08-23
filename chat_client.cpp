/****** 
			CPDP Project 02 -_A Simple Chat Application: Client Program
****/

/************* Header Files *************/
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <unordered_map>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <fstream>
#include <arpa/inet.h>
#include <netdb.h>

/*********** Macros ***********/
#define OPERATION_LOGIN "login"
#define OPERATION_LOGOUT "logout"
#define OPERATION_MESSAGE "message"
#define BUFFER_SIZE 4096
#define OP_DELIMITER '~'
#define MAIN_DELIMITER '|'
#define VALUE_DELIMITER ':'
#define VALUE_USERNAME "username"

using namespace std;

/*************  Functions *************/

void parseConfigurationFile(char *filename, string &servhost, int &servport);
int connectToRemoteMachine(const char *ip, int port,bool exitIferror);
void *processServerConnection(void *arg);
void parseServerMessage(int sockfd,char *buf);
string creatLoginPayloadString(string username);
string createLogoutPayloadString();
string createMessagePayloadString(string originalUser, string forUser, string message);
string createMessageToAllPayloadString(string originalUser, string messageString);
void initialPromtDisplayForUser();
void userAfterLoginPromt();
void signalHandlerSigInt(int signo);

bool userLoggedIn = false;
int serverSockfd;
int peerServport=-1;
const string loginCommand = "login ";
const string messageSendCommand = "chat ";



int main(int argc, char * argv[]) {

    pthread_t tid;
    int servport,*sock_ptr;
    string bufferString, password, payloadString, servhost;
    struct sigaction abc;
    abc.sa_handler = signalHandlerSigInt;
    sigemptyset(&abc.sa_mask);
    abc.sa_flags = 0;
    sigaction(SIGINT, &abc, NULL);
    if (argc < 2) {
        printf("Please provide client configuration file\n");
        exit(EXIT_SUCCESS);	
    }
    parseConfigurationFile(argv[1], servhost, servport);
    serverSockfd = connectToRemoteMachine(servhost.c_str(),servport,true);
    if(argc == 3) {
        peerServport = atoi(argv[2]);
    }
    sock_ptr = (int *)malloc(sizeof(int));
    *sock_ptr = serverSockfd;
    pthread_create(&tid, NULL, &processServerConnection, (void*)sock_ptr);

    istringstream inputMessageStream("");
    string forUser, originalUser;
    string messageString;
    initialPromtDisplayForUser();
    
/************* Command Options ****************/
    
    while (getline(cin, bufferString)) {
		
        /*************Logout from the Server****************/
        if(bufferString.compare("logout") == 0) {
			
            if(!userLoggedIn) {
                cout << "User is not logged in" << endl;
                continue;
            } 
			payloadString=createLogoutPayloadString();
			write(serverSockfd, payloadString.c_str(), strlen(payloadString.c_str())+1);
        }
        /*************Quit the Program****************/
        else if(bufferString.compare("exit") == 0) {
			
			if(userLoggedIn) {
				cout << "You cannot exit without being LoggedOut first, please logout to exit" << endl;
				continue;
			}
            close(serverSockfd);
            exit(0);
        }
        else { //login or chat
			/*************Login Command****************/
			if(strncmp(bufferString.c_str(),loginCommand.c_str(),strlen(loginCommand.c_str())) == 0) { 
			
				if(userLoggedIn) {
					cout << "You are already logged in" << endl;
					continue;
				}
				string username = "";
				//userLoggedIn=1;
                inputMessageStream.clear();
                inputMessageStream.str(bufferString);
				string command;
                getline(inputMessageStream,command,' '); //command='login' here
				getline(inputMessageStream,username,' '); //username value
				payloadString = creatLoginPayloadString(username);
				originalUser=username;
				write(serverSockfd, payloadString.c_str(), strlen(payloadString.c_str())+1);
			}
			/*************Message Command****************/
            else if(strncmp(bufferString.c_str(),messageSendCommand.c_str(),strlen(messageSendCommand.c_str())) == 0) {
				
				if(!userLoggedIn) {
					cout << "User is not logged in" << endl;
					continue;
				}
                inputMessageStream.clear();
                inputMessageStream.str(bufferString);
                getline(inputMessageStream,forUser,' '); //forUser='m' here
				bool check=0;
				if(bufferString[5]=='@') check=1;
				if(check)
				{
					getline(inputMessageStream,forUser,' '); //forUser gets updated if a @user is present
					forUser.erase(forUser.begin()+0);
					getline(inputMessageStream,messageString);
					payloadString = createMessagePayloadString(originalUser, forUser, messageString);
					write(serverSockfd, payloadString.c_str(), strlen(payloadString.c_str())+1);
				}
				else 
				{
					getline(inputMessageStream, messageString); //the message
					payloadString = createMessageToAllPayloadString(originalUser, messageString);
					write(serverSockfd, payloadString.c_str(), strlen(payloadString.c_str())+1);
				}
            }
        }
    }
}

/************* Prompt Messages ****************/

void initialPromtDisplayForUser() {
    cout << "1. Enter ' login username ' (Replace username with desired name) to login " << endl;
    cout << "2. Enter ' exit ' to quit the program" << endl;
    
}
void userAfterLoginPromt() {
    cout << "1. Enter ' chat @friend_username message ' to send message to a particular user" << endl;
	cout << "2. Enter ' chat message ' to send message to all" << endl;
	cout << "3. Enter 'logout' to logout from the server" << endl;
}


/************* Parse Server Message ****************/

void parseServerMessage(int sockfd,char *buf) {
    string sbuf(buf);
	cout<<"ServerMessage: "<<sbuf<<endl; //output the server message 
    istringstream serverMessage(sbuf);
    string messageBody,response;
    getline(serverMessage,messageBody,OP_DELIMITER);
    if(messageBody.compare(OPERATION_LOGIN) == 0) {
        getline(serverMessage,messageBody,OP_DELIMITER);
        if(messageBody.compare("200") == 0) {
			userLoggedIn = true; 
            cout << "Login successful." << endl;
			userAfterLoginPromt();

        }
        else {
            cout << "Login failed. Please try again" << endl;
        }

    }
	else if(messageBody.compare(OPERATION_LOGOUT) == 0) {
        getline(serverMessage,messageBody,OP_DELIMITER);
        if(messageBody.compare("200") == 0) {
			userLoggedIn = false; 
            cout << "User Successfully LoggedOut" << endl;
			initialPromtDisplayForUser();
        }
        else {
            cout << "Logout failed as user is not logged in." << endl;
        }

    }
	else if(messageBody.compare(OPERATION_MESSAGE) == 0) {
		string user,m;
		getline(serverMessage,messageBody,':');
		getline(serverMessage,messageBody,'|');
		getline(serverMessage,m,':');
		getline(serverMessage,m);
		cout<<messageBody<<" >> "<<m<<endl;
	}
    serverMessage.clear();
}


/************* Connect to Remote Machine ****************/

int connectToRemoteMachine(const char *ip, int port,bool exitIferrorIsTrue) {
    int sockfd = -1;
    int rv, isConnectflag;
    struct addrinfo hints, *res, *ressave;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(ip, to_string(port).c_str() , &hints, &res)) != 0) {
        cout << "getaddrinfo() wrong: " << gai_strerror(rv) << endl;
        if(exitIferrorIsTrue) {
            exit(EXIT_FAILURE);
        }
        return sockfd;
    }
    ressave = res;
    isConnectflag = 0;
    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
            isConnectflag = 1;
            break;
        }
        close(sockfd);
    }
    while ((res = res->ai_next) != NULL);
    freeaddrinfo(ressave);
    if (isConnectflag == 0) {
        fprintf(stderr, "Connect Error\n");
        if(exitIferrorIsTrue) {
            exit(1);
        }
    }
    return sockfd;
}
/************* Parse Client Configuration File ****************/

void parseConfigurationFile(char *filename, string &servhost, int &servport) {
    ifstream infile;
    infile.open(filename);
    string str, strbuf;
    for(int i = 0; i < 2; i++) {
        getline(infile,str);
        istringstream lineStream(str);
        getline(lineStream,strbuf,':');

        if(strbuf.compare("servhost") == 0) {
            getline(lineStream,servhost,':');
        }
        else if(strbuf.compare("servport") == 0) {
            getline(lineStream,strbuf,':');
            servport = stoi(strbuf);
        }
    }
}

/***************** Create Payloads String ******************/

string createMessagePayloadString(string originalUser, string forUser, string message) {
    ostringstream buffer;
    string payload;
	payload=string("message")+'~'+string("fromUser")+':'+originalUser+'|'+string("toUser")+':'+forUser+'|'+string("message")+':'+message;
    return payload;

}

string createMessageToAllPayloadString(string originalUser, string message)
{
	ostringstream buffer;
    string payload;
	payload=string("message")+'~'+string("fromUser")+':'+originalUser+'|'+string("message")+':'+message;
    return payload;
}

string creatLoginPayloadString(string username) {
    ostringstream buffer;
    string payload;
	buffer << OPERATION_LOGIN << OP_DELIMITER << VALUE_USERNAME << VALUE_DELIMITER << username;
    payload = buffer.str();
    buffer.clear();
    return payload;
}

string createLogoutPayloadString() {

    ostringstream buffer;
    string payload;
    buffer << OPERATION_LOGOUT;
    payload = buffer.str();
    buffer.clear();
    return payload;
}

/* Server Connection Process */

void *processServerConnection(void *arg) {
    int n;
    int sockfd;
    char buf[BUFFER_SIZE];
    sockfd = *((int *)arg);
    free(arg);
    pthread_detach(pthread_self());
    while (1) {
        n = read(sockfd, buf, BUFFER_SIZE);
        if (n == 0){
            printf("Server Not Found (Crashed)!!!\n");
            close(sockfd);
            exit(0);
        }
        parseServerMessage(sockfd, buf);
    }
}

/********* Ctrl-C Pressed: SIGINT ********/

void signalHandlerSigInt(int signo) {
    cout << endl << "SIGINT" << endl;
    close(serverSockfd);
    exit(0);
}
