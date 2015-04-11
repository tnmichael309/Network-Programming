#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

using namespace std;

#define MAX_CONNECTION_NUM 10

class server{
public:
	static void serverCloseHandler(int signal);
	static void clientCloseHandler(int signal);
	static void sigchldHandler(int signal);
	static int currentServerSocketFd;
	
	server(int portNum);
	~server();
	
private:
	void clientHandler(int sockfd);
	int catchSignal(int sig, void (*handler)(int));
};