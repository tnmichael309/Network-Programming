#ifndef _CLIENTJOB_H
#define _CLIENTJOB_H

#include <iostream>
#include <string.h>
#include <cstring>
#include <stdio.h>
#include <fstream>
#include <cctype>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sstream>
using namespace std;

class clientJob{
public:
	clientJob();
	void init(string ip, string port, string file, int jobID);
	~clientJob();
	void getServerInfo(string& ip, string& port);
	bool doAJob();
	string getResultString();
	void setConnectedSocketFD(int fd);
	void readUntilEnd(bool& isReadyToWrite);
	void addToResultString(string addedString);
private:
	string m_sSeverIP;
	string m_sServerPort;
	string m_sSeverFile;
	string m_resultString;
	int m_jobID;
	int m_serverFD;
	fstream m_batchFile;
};

#endif