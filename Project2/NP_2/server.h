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
#include "client.h"
#include "fifos.h"

using namespace std;
	
class server
{
public:
	static int staticListenerD;
	static void handleShutdown(int sig);
	static void handleClientClosed(int sig);

	server();
	~server();
	void start(int port);

private:
	void error(const char *msg);
	int openListenerSocket();
	void bindToPort(int socket, int port);
	int catchSignal(int sig, void (*handler)(int));
	int handleClient(int fd);
	
	// return -1 : no IPC command found
	// return other number : execute coded IPC command
	int isIPCCommand(string s);
	void exeuteIPCCommand(int commandSelect, string s, int curID);
	void who(int curID); 
	void name(int curID, string s);
	void tell(int curID, string s);
	void yell(int curID, string s);
	
	// communication
	void sendMessage(int curID, int tarID, string s);
	void sendMessage(int tarID, string s);
	void broadcastMessage(int curID, string s);
	void broadcastMessageFromServer(int curID, string s);
	bool redirectToOtherUser(string& s, int curID, int& readFD, int& writeFD, string& readRedirectMessage, string& writeRedirectMessage, string& falseMessage);
	string reviseString(string s);
	
	clientPools m_clientPools;
	FIFOS m_fifos;
};

#endif