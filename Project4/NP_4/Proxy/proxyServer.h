#ifndef __PROXYSERVER_H__
#define __PROXYSERVER_H__

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <iomanip>
using namespace std;

#define BIND_MODE_PORT 5180

class server
{
public:
	static int staticListenerD;
	static void handleShutdown(int sig);
	static void handleClientClosed(int sig);
	static void sigchldHandler(int sig);
	
	server();
	~server();
	void start(int port, int constraint);

private:
	void error(const char *msg);
	int openListenerSocket();
	void bindToPort(int socket, int port);
	int catchSignal(int sig, void (*handler)(int));
	int handleClient(int fd);
	
	void connectModeHandler(int sockfd, string ip, string port, unsigned char* SOCKS4_REPLY);
	void bindModeHandler(int sockfd, string ip, string port);
public:	
	int currentSocketFD;
	
	int constraintType;
	string srcIP;
	string srcPort;
};

#endif