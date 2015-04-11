#ifndef _CLIENT_HANDLER_H
#define _CLIENT_HANDLER_H

#include "clientJob.h"
#include <windows.h>
#include <list>
#include <cstring>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <fcntl.h>
using namespace std;

class singleClient{
public:
	singleClient(){
		currentUserNum = 0;
		serverSocketFD = 0;
	}
	~singleClient(){;}
	
	void init(SOCKET server){
		serverSocketFD = server;
	}

	clientJob& getANewJob(){
		int lastUserNum = currentUserNum;
		currentUserNum ++ ;
		return aClientJobs[lastUserNum];
	}

	clientJob& getAnExisJob(int i){
		
		return aClientJobs[i];
	}

	int getUserNum(){
		return currentUserNum;
	}

	SOCKET serverSocketFD;
	int currentUserNum;
	clientJob aClientJobs[5];
	bool isActive;
};

class clientHandler{
public:

	void addNewClient(SOCKET serverfd){
		vClients.push_back(new singleClient());
		vClients[vClients.size() -1]->init(serverfd);
	}

	void deleteClient(SOCKET serverfd){

		int IDtoErase = -1;
		for(int i = 0; i < vClients.size(); i++){
			if(vClients[i]->serverSocketFD == serverfd){
				IDtoErase = i;
				break;
			}
		}

		if(IDtoErase == -1);
		else{
			vClients.erase(vClients.begin() + IDtoErase);
		}
	}

	singleClient* findClient(SOCKET serverfd){
		for(int i = 0; i < vClients.size(); i++){
			if(vClients[i]->serverSocketFD == serverfd)
				return vClients[i];
		}
		return NULL;
	}

	bool isClientExist(SOCKET serverfd){
		for(int i = 0; i < vClients.size(); i++){
			if(vClients[i]->serverSocketFD == serverfd)
				return true;
		}
		return false;
	}
	vector<singleClient*> vClients;
};

#endif