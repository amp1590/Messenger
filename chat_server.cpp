/****** CPDP Project 02 -_A Messenger Application: Server Program *****/

/************* Headers *************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unordered_map>

/************* Macros *************/

#define BUFFER_SIZE 4096
#define OPERATION_LOGIN "login"
#define OPERATION_LOGOUT "logout"
#define VALUE_USERNAME "username"
#define OPERATION_MESSAGE "message"
#define OPERATION_EXIT "exit"
#define MAIN_DELIMITER  '|'
#define VAL_DELIMITER  ':'
#define OP_DELIMITER '~'
#define VALUE_PORT "port"


using namespace std;

/*************  Function Prototypes  *************/

int parsePortFromConfigFile(char * configFileName);
void signalHandlerSigInt(int signo);
void removeFromOnlineUsersUnorderedMap(int fd);
bool loginUser(int sockfd, string messageBody); 
bool logoutUser(int sockfd, string messageBody);
void exitUser(string messageBody);
bool parseClientMessage(int sockfd,char *buf);
string createLoginLogoutResponseString(string opType,string statusCode);
void checkHostName(char hostname[]);
void printServerInfo(char hostname[]);

/************* Globals *************/

int sockfd, rec_sock;
fd_set all_select_set;
vector <int> all_sockets_vector;
unordered_map<string, int> aru_all_users_sock;
unordered_map<int, bool> all_sock_logout;

/************* Main *************/

int main(int argc, char * argv[]) {
    socklen_t len;
    struct sockaddr_in addr, recaddr;
    char buf[BUFFER_SIZE];
    fd_set readset;
    int maximumfd;
    int port;
    struct sigaction abc;
    abc.sa_handler = signalHandlerSigInt;
    sigemptyset(&abc.sa_mask);
    abc.sa_flags = 0;
    sigaction(SIGINT, &abc, NULL);
	aru_all_users_sock.clear();
    port = parsePortFromConfigFile(argv[1]);
    
    if (argc < 2) {
        printf("Please provide the server configuration file\n");
        exit(0);
    }
	
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(": Socket not found");
        exit(1);
    }
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((short)port);
    if (::bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { // if mac ::bind else bind check
        perror(": bind");
        exit(1);
    }
    char hostname[128];
    if(gethostname(hostname,128) == -1) {
        perror("Failed to get hostname");
        exit(EXIT_FAILURE); 
    }
    gethostbyname(hostname);
    checkHostName(hostname);
    printServerInfo(hostname);
    len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr *)&addr, &len) < 0) { // motive
      perror(":Failed to get name");
      _exit(1);
   }
    printf("\nPort:%d\n", htons(addr.sin_port)); //printing port of server
    if (listen(sockfd, 5) < 0) {
        perror(": bind");
        exit(1);
    }
    // fd_set for select
    FD_ZERO(&all_select_set);
    FD_SET(sockfd, &all_select_set);
    all_sockets_vector.clear();
    maximumfd = sockfd;
	//Collecting socket information of each client
    while (1) {
        readset = all_select_set;
        select(maximumfd+1, &readset, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &readset)) {
            len = sizeof(recaddr);
            if ((rec_sock = accept(sockfd, (struct sockaddr *)(&recaddr), &len)) < 0) {
                if (errno == EINTR)
                    continue;
                else {
                    perror(":accept error");
                    exit(1);
                }
            }
            if (rec_sock < 0) {
                perror(": accept");
                exit(1);
            }
		    //Just after running the client program
            printf("Connected Remote Machine = %s | Port = %d.\n",inet_ntoa(recaddr.sin_addr), ntohs(recaddr.sin_port));
            
            all_sockets_vector.push_back(rec_sock);
            FD_SET(rec_sock, &all_select_set);
            if (rec_sock > maximumfd) maximumfd = rec_sock;
        }
        auto iter = all_sockets_vector.begin();
        while (iter != all_sockets_vector.end()) {
            int num, fd;
            fd = *iter;
            if (FD_ISSET(fd, &readset)) {
                num = read(fd, buf, BUFFER_SIZE);
                if (num == 0) { // why num == 0 deletes the user from the list
                    close(fd);
                    FD_CLR(fd, &all_select_set);
                    iter = all_sockets_vector.erase(iter);
                    removeFromOnlineUsersUnorderedMap(fd);
                    continue;
                }
                else {
                    bool islogout = parseClientMessage(fd,buf);
                    if(islogout) {
                        continue;
                    }
                }
            }
            ++iter;
        }
        maximumfd = sockfd;
        if (!all_sockets_vector.empty()) {
            maximumfd = max(maximumfd, *max_element(all_sockets_vector.begin(), all_sockets_vector.end())); // could write a method
        }
    }
}


/************* Parse Client Message *************/

bool parseClientMessage(int sockfd,char *bufferstr) { 

    string strbuf(bufferstr);
    istringstream message(strbuf);
    string messageBody,response;
    cout << "Client message :" << strbuf << endl << flush;
    getline(message,messageBody,OP_DELIMITER);
	if(messageBody.compare(OPERATION_LOGIN) == 0) {
        getline(message,messageBody,OP_DELIMITER);
        if(loginUser(sockfd, messageBody)) { 
            response = createLoginLogoutResponseString(OPERATION_LOGIN,"200");//return status code "200" since the username is available 
        } else {
            response = createLoginLogoutResponseString(OPERATION_LOGIN,"500");//return status code "500" since the username is unavailable
        }
        write(sockfd,response.c_str(),strlen(response.c_str())+1);
    }
    else if(messageBody.compare(OPERATION_LOGOUT) == 0) {
        if(logoutUser(sockfd, messageBody)) { 
            response = createLoginLogoutResponseString(OPERATION_LOGOUT,"200");//return status code "200" since the user was found 
        } else {
            response = createLoginLogoutResponseString(OPERATION_LOGOUT,"500");//return status code "500" since the user was not found
        }
        write(sockfd,response.c_str(),strlen(response.c_str())+1);
        return true;
    }
	else if(messageBody.compare(OPERATION_MESSAGE) == 0){
		string bodyOfMsg,firstPart,secondPart,thirdPart,toUserTag,toUser,fromUser,fromUserTag,sendMessageTag,sendMessage;	
		getline(message,bodyOfMsg,'~');
		stringstream msgStream(bodyOfMsg); //'message'
		getline(msgStream,firstPart,'|'); //upto original user
		getline(msgStream,secondPart,'|'); //upto forUser
		getline(msgStream,thirdPart,'|'); //upto last
		stringstream firstPartSream(firstPart);
		stringstream secondPartSream(secondPart);
		stringstream thirdPartSream(thirdPart);
		getline(firstPartSream,fromUserTag,':'); //the word "fromUser"
		getline(firstPartSream,fromUser,':'); //the value OF fromUser
		getline(secondPartSream,toUserTag,':'); //the word "toUser"
		bool check_all_user=0;
		if(toUserTag=="toUser")
		{
			getline(secondPartSream,toUser,':'); //the value of toUser
			getline(thirdPartSream,sendMessageTag,':'); //the word "message"
			getline(thirdPartSream,sendMessage,':'); //the value of message
		}
		else if(toUserTag=="message")
		{
			check_all_user=1;
			sendMessageTag=toUserTag; //the word "message"
			getline(secondPartSream,sendMessage,':'); //the value of message
		}
		msgStream.clear();
		msgStream.str("");
		firstPartSream.clear();
		firstPartSream.str("");
		secondPartSream.clear();
		secondPartSream.str("");
		thirdPartSream.clear();
		thirdPartSream.str("");
		string response;
		response.clear();
		
		if(check_all_user==0)
		{
			auto it=aru_all_users_sock.find(toUser);
			if(it!=aru_all_users_sock.end())
			{
				int receiverSocket=it->second;
				string receiverResponse=string("message")+'~'+string("fromUser")+':'+fromUser+'|'+string("message")+':'+sendMessage;
				write(receiverSocket, receiverResponse.c_str(),strlen(receiverResponse.c_str())+1);
			}
		}
		else
		{
			for(auto it=aru_all_users_sock.begin(); it!=aru_all_users_sock.end(); ++it)
			{
				if(it->first==fromUser) continue;
				int receiverSocket=it->second;
				string receiverResponse=string("message")+'~'+string("fromUser")+':'+fromUser+'|'+string("message")+':'+sendMessage;
				write(receiverSocket, receiverResponse.c_str(),strlen(receiverResponse.c_str())+1);
			}
		}
		
        message.clear();
		message.str("");
	}
	else if(messageBody.compare(OPERATION_EXIT) == 0){
		getline(message,messageBody,OP_DELIMITER);
        exitUser(messageBody);
	}
    message.clear();
    return false;
}

/*************  Update Online User List  *************/

void removeFromOnlineUsersUnorderedMap(int fd) { //Done
	for(auto it=aru_all_users_sock.begin(); it!=aru_all_users_sock.end(); ++it)
    {
        if(it->second == fd){
            it = aru_all_users_sock.erase(it);
            cout<<"Current Online Users Count: " << aru_all_users_sock.size() << endl;
            return;
        } 
    }
}

/******* Logout User ******/

bool logoutUser(int sockfd, string messageBody) { //Done
    for(auto it=aru_all_users_sock.begin(); it!=aru_all_users_sock.end(); ++it)
    {
        if(it->second == sockfd){
            it = aru_all_users_sock.erase(it);
            cout<<"Current Online Users Count: " << aru_all_users_sock.size() << endl;
            return true;
        } 
    }
	return false;
}

void exitUser(string messageBody) { 
    string s,username;
    istringstream mesageBodyStream(messageBody);
    istringstream valueStream("");
    while(getline(mesageBodyStream,s,MAIN_DELIMITER)) {
        valueStream.clear();
        valueStream.str(s);
        getline(valueStream,s,VAL_DELIMITER);
        if(s.compare(VALUE_USERNAME) == 0) {
            getline(valueStream,username,VAL_DELIMITER); //this username is the actual username value
        }
    }
	
	auto it=aru_all_users_sock.find(username);
	if(it!=aru_all_users_sock.end())
	{
		aru_all_users_sock.erase(it);
	}
	close(rec_sock);
}

/************ User Login ************/

bool loginUser(int sockfd, string messageBody) { //addition of sockfd
    string s,username,password;
    istringstream mesageBodyStream(messageBody);
    istringstream valueStream("");
    while(getline(mesageBodyStream,s,MAIN_DELIMITER)) {
        valueStream.clear();
        valueStream.str(s);
        getline(valueStream,s,VAL_DELIMITER);
        if(s.compare(VALUE_USERNAME) == 0) {
            getline(valueStream,username,VAL_DELIMITER);
        }
    }
    mesageBodyStream.clear();
    valueStream.clear();
	cout << "Login Successful for user " <<username<< endl;
	aru_all_users_sock[username]=sockfd;
	//for(auto it=aru_all_users_sock.begin(); it!=aru_all_users_sock.end(); ++it) cout<<"user: "<<it->first<<" socket: "<<it->second<<endl;
	return true;
    
 
}
/*********** Create Login And Logout Response String ***********/

string createLoginLogoutResponseString(string opType,string statusCode) { //done
    ostringstream bufferstr;
    string payload;
    bufferstr<<opType<<OP_DELIMITER<<statusCode;
    payload = bufferstr.str();
    bufferstr.clear();
    return payload;
}



/************ Getting Port from Config File ************/

int parsePortFromConfigFile(char *configFileName) {
    ifstream infile;
    infile.open(configFileName);
    string str;
    getline(infile,str,VAL_DELIMITER);
	cout<<str<<endl;
    if(str.compare(VALUE_PORT) == 0) {
        getline(infile,str,VAL_DELIMITER);
    }
    else {
        cout <<"Error in Configuration. Port not found" << flush;
        exit(1);
    }
    infile.close();
    return stoi(str);

}



/************ Ctrl-C: SIGINT ************/

void signalHandlerSigInt(int signo) {
    cout << endl << "SIGINT" << endl;
    //saveUserInfoFile(); on chilo
    auto itr = all_sockets_vector.begin();
    while (itr != all_sockets_vector.end()){
        int fd;
        fd = *itr;
        close(fd);
        ++itr;
    }
    close(sockfd);
    exit(0);
}


void checkHostName(char hostname[]) {
    int len = strlen(hostname);
    for (int i = 0; i < len; i++) {
        //cout << "hostname[" << i << "]" << " ("<< hostname[i] << ")" << endl;
        if (hostname[i] == '.') {
            hostname[i] = '\0';
            break;
        }
    }
}

void printServerInfo(char hostname[]) {
    cout << "Server Started" << endl;
    cout << "Server: " << hostname;
    return;
}

