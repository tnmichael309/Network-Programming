#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sstream>
#include <vector>
#include <cstring>
#include <string.h>
#include <fcntl.h>

#define MAXUSER 5
#define EXIT_STR "exit\r\n"
using namespace std;

#include "clientJob.h"


static void splitString(string originalString, vector<string>& resultString){

	stringstream ss(originalString);
	string subString;
	while(getline(ss,subString,'&')) // divide string by '&'
		resultString.push_back(subString);	

	return;
}

void compileCFile(string sQueryString){
	string compileCommand = "gcc ";
	compileCommand += sQueryString;
	compileCommand += " && ./a.out\n";
	
	//cout << compileCommand << endl;
	cout << "<html> <head>" << endl;
	setenv("PATH","/bin/:.",1);
	system(compileCommand.c_str());
	cout << "</head></html>" << endl;
}

int main(){

	fd_set              rfds, afds;
	int                 unsend, len, SERVER_PORT[MAXUSER], i;
	int                 client_fd[MAXUSER] = {-1,-1,-1,-1,-1};
	struct sockaddr_in  client_sin[MAXUSER];
	struct hostent		*he[MAXUSER];
	
	clientJob* aClientJobs;
	aClientJobs = new clientJob[MAXUSER];
	
	// test use
	setenv("QUERY_STRING", "support.c",1);
	
	string sQueryString(getenv("QUERY_STRING"));
	
	if(sQueryString.find(".c") != string::npos){
		compileCFile(sQueryString);
		return 0;
	}
	
	vector<string> vDividedStrings;
	
	// split query strings
	splitString(sQueryString, vDividedStrings);
	
	// initialize all jobs
	for(int i = 0; i < vDividedStrings.size(); i+=3){
	
		int userno = i/3;
		
		// if no server, continue;
		if(vDividedStrings[i].find_first_of('=') == vDividedStrings[i].size() - 1){
			aClientJobs[userno].init("", "", "", userno);
			client_fd[userno] = -1;
			continue;
		}
		
		string sServerIP = vDividedStrings[i].substr(vDividedStrings[i].find_first_of('=')+1);
		string sServerPort = vDividedStrings[i+1].substr(vDividedStrings[i+1].find_first_of('=')+1);
		string sServerFile = vDividedStrings[i+2].substr(vDividedStrings[i+2].find_first_of('=')+1);
		
		//cout << sServerIP << " " << sServerPort << " " << sServerFile << endl;
		
		// initialize this job
		he[userno] = gethostbyname(sServerIP.c_str());
		SERVER_PORT[userno] = atoi(sServerPort.c_str());
		client_fd[userno] = socket(AF_INET,SOCK_STREAM,0);
		
		aClientJobs[userno].init(sServerIP, sServerPort, sServerFile, userno);
		aClientJobs[userno].setConnectedSocketFD(client_fd[userno]);
	}
	
	// handle connections
	for(int i = 0; i < MAXUSER; i++){
		if(client_fd[i] == -1) continue;
		memset(&client_sin[i], 0, sizeof(client_sin[i])); 
		client_sin[i].sin_family = AF_INET;
		client_sin[i].sin_addr = *((struct in_addr *)he[i]->h_addr); 
		client_sin[i].sin_port = htons(SERVER_PORT[i]);
	}
	
	
	// show html
	cout << "<html>"
        << "<head>"
        << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />"
        << "<title>Network Programming Homework 3</title>"
        << "</head>"
        << "<body bgcolor=#336699>"
        << "<font face=\"Courier New\" size=2 color=#FFFF99>"
        << "<table width=\"800\" border=\"1\">"
        << "<tr>";
	for(int i = 0; i < MAXUSER; i++){
		string serverIP;
		string serverPort;
		aClientJobs[i].getServerInfo(serverIP,serverPort);
		
		if(serverIP == "" || serverPort == "") continue;
		else cout << "<td>" << serverIP << "</td>";
	}
	cout << "</tr><tr>";
	for(int i = 0; i < MAXUSER; i++){
		string serverIP;
		string serverPort;
		aClientJobs[i].getServerInfo(serverIP,serverPort);
		
		if(serverIP == "" || serverPort == "") continue;
		else cout << "<td valign=\"top\" id=\"m" << i << "\"></td>";
	}
	cout << "</tr></table>"<<endl;
	
	
	// handle fds
	FD_ZERO(&rfds);
	FD_ZERO(&afds);
	
	bool isSendCommand[MAXUSER] = {false,false,false,false,false};
	bool isReadyToWrite[MAXUSER] = {false,false,false,false,false};
	
	// set non-blocking
	int maxfd = 0;
		
	// connect to server now
	for(int i = 0; i < MAXUSER; i++){
		if(client_fd[i] == -1) continue;
		
		// setting non-blocking socket
		int flags;
		if (-1 == (flags = fcntl(client_fd[i], F_GETFL, 0)))
			flags = 0;
		fcntl(client_fd[i], F_SETFL, flags | O_NONBLOCK);
		
		// find max fd
		if(client_fd[i] > maxfd) maxfd = client_fd[i];
		
		// keep connecting sockfd
		while(1){
			if(connect(client_fd[i],(struct sockaddr *)&client_sin[i],sizeof(client_sin[i])) == -1) {
				//cerr << "connect fail for user#: " << i << "\n";
			}else{
				//cerr << "connect success for user#: " << i << "\n";
				break; // connect successfully
			}
		}	
		// connect succesfully and set afds
		FD_SET(client_fd[i], &afds);
	}
	
	while(1){
		memcpy(&rfds, &afds, sizeof(fd_set));
		if(select(maxfd+1,&rfds,NULL,NULL,NULL) < 0) return 0;
		
		
		for(i=0; i<MAXUSER; i++)
		{  
			// update active status
			if(client_fd[i] == -1) continue;
	
			if(FD_ISSET(client_fd[i], &rfds))
			{
				// read then write
				aClientJobs[i].readUntilEnd(isReadyToWrite[i]);	
				cout << aClientJobs[i].getResultString() << endl;
			}
			
			if(isReadyToWrite[i]){
				bool isLast = aClientJobs[i].doAJob() == false;
					
				sleep(1);
				
				if(isLast){
					sleep(4);
					aClientJobs[i].readUntilEnd(isReadyToWrite[i]);	
					
					cout << aClientJobs[i].getResultString() << endl;
					
					
					close(client_fd[i]);
					FD_CLR(client_fd[i], &afds); 
					client_fd[i] = -1;
				}
				else{
					cout << aClientJobs[i].getResultString() << endl;
					isReadyToWrite[i] = false;
				}
			}
			
		}
		
		bool isStillActive = false;
		for(int i = 0; i < MAXUSER; i++){
			if(client_fd[i] != -1){
				isStillActive = true;
				break;
			}
		}
		if(!isStillActive) goto finish_while;
		
	}
finish_while:
	
	/*for(int i = 0; i < MAXUSER; i++){
		string serverIP;
		string serverPort;
		aClientJobs[i].getServerInfo(serverIP,serverPort);
		
		if(serverIP == "" || serverPort == "") continue;
		else cout << aClientJobs[i].getResultString() << endl;
	}*/
	cout << "</font>"
        << "</body>"
        << "</html>"<<endl;
		
	delete[] aClientJobs;	
	
	return 0;
}