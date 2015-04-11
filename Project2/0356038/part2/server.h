#ifndef __SERVER_H__
#define __SERVER_H__

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
#include <sys/ipc.h>
#include <sys/shm.h>

#include "client.h"
#include "fifos.h"

using namespace std;

typedef struct{
	bool isSuccess;
	int srcSocketFd;
	int srcPid;
	int numberOfMessage;
	char message[5][1024];
} Mesg;

static void copyStringToCharArray(string s, char*);

class server
{
public:
	static int staticListenerD;
	static void handleShutdown(int sig);
	static void handleClientClosed(int sig);
	static void sigchldHandler(int sig);
	static void sigusrHandler(int sig);
	
	server();
	~server();
	void start(int port);

private:
	void error(const char *msg);
	int openListenerSocket();
	void bindToPort(int socket, int port);
	int catchSignal(int sig, void (*handler)(int));
	int handleClient(int fd);
	void deleteUser(int newsockfd);

	// return -1 : no IPC command found
	// return other number : execute coded IPC command
	int isIPCCommand(string s);
	void exeuteIPCCommand(int commandSelect, string s, int curID);
	
	void whoServer(int srcFD); 
	void whoClient(int curID); 
	
	void nameServer(int srcFD, string s);
	void nameClient(int curID, string s);
	
	void tellServer(int srcFD, string s);
	void tellClient(int curID, string s);
	
	void yellServer(int srcFD, string s);
	void yellClient(int curID, string s);
	
	// communication
	void sendMessage(int curID, int tarID, string s);
	void sendMessage(int tarID, string s);
	void broadcastMessage(int curID, string s);
	void broadcastMessageFromServer(int curID, string s);
	bool redirectToOtherUser(string s, int curID);
	bool redirectClient(string& s, int curID, int& readFd, int& writeFd, string& readRedirectMessage, string& writeRedirectMessage, string& falseMessage);
	string reviseString(string s);
	
public:	
	int currentSocketFD;
	clientPools m_clientPools;
	FIFOS m_fifos;
};

#endif