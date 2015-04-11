#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <cstring>
#include <iostream>
#include <vector>
#include "shell.h"
using namespace std;

class singleClient{
public:
	singleClient();
	~singleClient();
	void init(string targetName, string ip, string port, int sockfd);
	void setName(string name);
	string getName();
	string getIP();
	string getPort();
	Shell* getShell();
	int getSockfd();
	bool isActiveUser();
	bool isNewUser();
	void setAvtiveStatus(bool status);
	void setOldUser();
	void setSelfEnvironment();
	void storeEnvironment();
	
private:
	string m_name;
	string m_ip;
	string m_port;
	int m_sockfd;
	bool m_isActive; // can be reused and replaced with other log-in user
	bool m_isNewUser;
	vector<string> m_environ;
	Shell* m_shell;
};

class clientPools{
public:
	// return user id
	int addUser(string ip, string port, int sockfd);
	
	// delete user id
	void delUser(int id);
	
	// set desired name using its own id
	bool setName(int id, string name);
	
	int findUserByFD(int fd);
	int findUserFD(int curID);
	int getContainerSize();
	bool isActiveUser(int id);
	
	bool isNewUser(int curID);
	string getUserName(int curID);
	string getUserIP(int curID);
	string getUserPort(int curID);
	Shell* getClientShell(int curID);
	void restoreClientShell(int curID);
	void showAllUsers(int curID);
	
	
private:
	vector<singleClient> m_vClients;
};

#endif