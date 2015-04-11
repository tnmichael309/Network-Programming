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
#include <algorithm>
#include <string>

using namespace std;

#include "resource.h"

#define SERVER_PORT 6464

#define WM_SOCKET_NOTIFY (WM_USER + 1)
#define WM_SOCKET_NOTIFY_SERVER (WM_USER + 2)
HWND hwndEdit;
BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf (HWND, TCHAR *, ...);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

class clientJob{
public:
	void init(string ip, string port, string file, int jobID, SOCKET sock){
		m_sSeverIP = ip;
		m_sServerPort = port;
		m_sSeverFile = file;
		m_jobID = jobID;
		m_resultString = "";
		m_serverSocket = sock;
		isFinish = FALSE;
		isReadyToWrite = FALSE;
		m_lastFilePosition = 0;
		//m_batchFile.open(file.c_str(), ios::in);

	}
	string getResultString(){
		string lastString = m_resultString;
		m_resultString = "";
		return lastString;
	}
	void addToResultString(string addedString){
		while(addedString != "" && addedString[addedString.size()-1] == 13) addedString.pop_back();

		m_resultString += "<script>document.all['m";
		m_resultString += to_string((unsigned long long)m_jobID);
		m_resultString += "'].innerHTML += \"";
		m_resultString += addedString;
		m_resultString += "<br>\";</script>\n";	
	}
	void clientJob::doAJob(){
		// do job according to each command
		// and wrap in m_resultString
		string command = "";

		ifstream m_batchFile(m_sSeverFile.c_str());
		
		m_batchFile.seekg(m_lastFilePosition);

		// do command in batch file
		if(getline(m_batchFile, command)){
			m_lastFilePosition += command.size() + 1;

			for(int i = command.size() -1; i > 0; i--){
				if(command[i] == 13) command.pop_back();
			}

			if(command != ""){
				// wrap command append to m_resultString
				string wrappedCommand = "% ";
				wrappedCommand += "<b>";
				wrappedCommand += command;
				wrappedCommand += "</b>";
				addToResultString(wrappedCommand);
			}

			// send command to server
			//cerr << m_jobID << " " << command << " " << command.size() << endl;
			command += "\n";
			send(m_serverSocket, command.c_str(), command.size(),0);

			if(command.find("exit") != string::npos) isFinish = true;
			else isFinish = false;
		}else{
			isFinish = true;
		}

	}
	void clientJob::readUntilEnd(){
		char* buff;
		buff = new char[4096];
		memset(buff, 0, 4096*sizeof(char));

		while(recv(m_serverSocket, buff, 4096, 0) > 0){
			string rString(buff);
			//cerr << "receive string: " << rString << endl;

			if(rString.find('%') != string::npos) isReadyToWrite = true;

			stringstream ss(rString);
			string subString;
			while(getline(ss,subString,'\n')){ // divide string by '\n'

				subString = ReplaceAll(subString, "\"", "&quot");
				subString = ReplaceAll(subString, ">", "&gt");
				subString = ReplaceAll(subString, "<", "&lt");
				subString = ReplaceAll(subString, "%", "");
				subString = ReplaceAll(subString, "\r", "");
				subString = ReplaceAll(subString, "\n", "");

				if(subString != " "){
					addToResultString(subString);
				}
			}
		}
	}
	string m_sSeverIP;
	string m_sServerPort;
	string m_sSeverFile;
	string m_resultString;
	int m_jobID;
	SOCKET m_serverSocket;
	streampos m_lastFilePosition;
	bool isFinish;
	bool isReadyToWrite;
};

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

	clientJob* getANewJob(){
		int lastUserNum = currentUserNum;
		currentUserNum ++ ;
		return &aClientJobs[lastUserNum];
	}

	clientJob* getAnExisJob(int i){

		return &aClientJobs[i];
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
		singleClient sC;
		sC.init(serverfd);
		vClients.push_back(sC);
	}

	void deleteClient(SOCKET serverfd){

		int IDtoErase = -1;
		for(int i = 0; i < vClients.size(); i++){
			if(vClients[i].serverSocketFD == serverfd){
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
			if(vClients[i].serverSocketFD == serverfd)
				return &vClients[i];
		}
		return NULL;
	}

	bool isClientExist(SOCKET serverfd){
		for(int i = 0; i < vClients.size(); i++){
			if(vClients[i].serverSocketFD == serverfd)
				return true;
		}
		return false;
	}
	vector<singleClient> vClients;
};

clientHandler globalClientHandler;

static void splitString(string originalString, vector<string>& resultString){

	stringstream ss(originalString);
	string subString;
	while(getline(ss,subString,'&')) // divide string by '&'
		resultString.push_back(subString);	

	return;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	
	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}

BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WSADATA wsaData;

	static SOCKET msock, ssock;
	static struct sockaddr_in sa;

	int err;


	switch(Message) 
	{
		case WM_INITDIALOG:
			hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_LISTEN:

					WSAStartup(MAKEWORD(2, 0), &wsaData);

					//create master socket
					msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

					if( msock == INVALID_SOCKET ) {
						EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
						WSACleanup();
						return TRUE;
					}

					err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

					if ( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(msock);
						WSACleanup();
						return TRUE;
					}

					//fill the address info about server
					sa.sin_family		= AF_INET;
					sa.sin_port			= htons(SERVER_PORT);
					sa.sin_addr.s_addr	= INADDR_ANY;

					//bind socket
					err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
						WSACleanup();
						return FALSE;
					}

					err = listen(msock, 2);
		
					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\r\n"));
						WSACleanup();
						return FALSE;
					}
					else {
						EditPrintf(hwndEdit, TEXT("=== Server START ===\r\n"));
					}

					break;
				case ID_EXIT:
					EndDialog(hwnd, 0);
					break;
			};
			break;

		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;

		case WM_SOCKET_NOTIFY:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_ACCEPT:
					ssock = accept(msock, NULL, NULL);
					Socks.push_back(ssock);
					EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\r\n"), ssock, Socks.size());
					EditPrintf(hwndEdit,TEXT("new coming socket: %d\r\n"),ssock);

					
					break;
				case FD_READ:
					//Write your code for read event here.

					for (std::list<SOCKET>::iterator it=Socks.begin(); it != Socks.end(); ++it){
						// receive request of the newest socket
						char* recvbuffer;
						recvbuffer = new char[1024];

						int bytesReceived;
						bytesReceived = recv(*it, recvbuffer, 1024*sizeof(char), 0);

						/*char* tempBuff;
						tempBuff = new char[1024];
						while(recv(ssock, tempBuff, 1024*sizeof(char), 0) > 0);
						delete[] tempBuff;*/

						EditPrintf(hwndEdit, TEXT("request: "));
						EditPrintf(hwndEdit, TEXT(recvbuffer));
						EditPrintf(hwndEdit, TEXT("\r\n"));

						// deal with request
						if(bytesReceived > 0){
							string receiveMessage;
							string requestType;
							string queryString;

							receiveMessage = string(recvbuffer);

							if(receiveMessage.find("GET") == string::npos) continue; // invalid request
							else{
								if(globalClientHandler.isClientExist(*it)){ // already dealt client
									continue;
								}
								else{ // possibly a new client
									
								}
							}

							stringstream ss(receiveMessage);
							string subString;
							if(getline(ss,subString,'\n')) receiveMessage = subString;	

							stringstream sss(receiveMessage);
							if(getline(sss,subString,'/') && getline(sss,subString,' ')) requestType = subString;

							if(requestType.find(".htm")!=string::npos);
							else if(requestType.find("cgi")!=string::npos){
								cerr << "original request: " << requestType << endl;

								stringstream ssss(requestType);
								getline(ssss,subString,'?');
								requestType = subString;

								cerr << "new request: " << requestType << endl;

								getline(ssss,subString,' ');
								queryString = subString;

								cerr << "query string: " << queryString << endl;
							}else continue;

							if(requestType.find("htm")!=string::npos){
								ifstream htmlFile(requestType.c_str());

								string webpage = string("<!DOCTYPE html>");
								if (htmlFile.is_open())
								{
									char c;
									while (htmlFile.get(c))          // loop getting single characters
										webpage += c;
									htmlFile.close();
								}else{
									webpage = "<html><head>404 Not Found : Invalid web page requested.</head><body></body></html>";
								}
								send(*it,webpage.c_str(),webpage.size(),0);
								continue;
							}else if(requestType.find("cgi")!=string::npos){
								globalClientHandler.addNewClient(*it);
								EditPrintf(hwndEdit, TEXT("adding new client: %d\r\n"),*it);
							}else{
								continue;
							}
							//cout << requestType << "\t" << queryString << endl;
							vector<string> vDividedStrings;

							// split query strings into vDividedStrings
							splitString(queryString, vDividedStrings);

							// handling multipel batches
							bool isInvalid = false;
							for(int i = 0; i < vDividedStrings.size(); i+=3){
								if(vDividedStrings[0].find_first_of('=') == vDividedStrings[0].size() - 1){
									isInvalid = true;
									break;// invalid request
								}else{
									string sServerIP = vDividedStrings[i+0].substr(vDividedStrings[i+0].find_first_of('=')+1);
									string sServerPort = vDividedStrings[i+1].substr(vDividedStrings[i+1].find_first_of('=')+1);
									string sServerFile = vDividedStrings[i+2].substr(vDividedStrings[i+2].find_first_of('=')+1);


									// connect to server : non-blocking

									// initialize winsocket
									int iResult;
									WSADATA wsaData;
									iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
									if (iResult != NO_ERROR) {
										printf("WSAStartup failed: %d\n", iResult);
										EditPrintf(hwndEdit, TEXT("=== win socket start up fail ===\r\n"));
										return 1;
									}

									// create a socket
									SOCKET m_socket;
									m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
									if (m_socket == INVALID_SOCKET) {
										EditPrintf(hwndEdit, TEXT("=== create socket fail ===\r\n"));
										WSACleanup();
										return FALSE;
									}

									// set server info
									struct hostent		*he;
									he = gethostbyname(sServerIP.c_str());
									if(he == NULL){
										EditPrintf(hwndEdit, TEXT("=== error: getting host name ===\r\n"));
										WSACleanup();
										return FALSE;
									}
									SOCKADDR_IN  client_sin;
									memset(&client_sin, 0, sizeof(client_sin)); 
									client_sin.sin_family = AF_INET;
									client_sin.sin_addr.s_addr = *((unsigned long long *)he->h_addr); 
									client_sin.sin_port = htons(atoi(sServerPort.c_str()));
									EditPrintf(hwndEdit, TEXT("=== finish setting server ===\n"));

									// add to async event
									err = WSAAsyncSelect(m_socket, hwnd, WM_SOCKET_NOTIFY_SERVER,FD_CLOSE | FD_READ | FD_WRITE);

									if ( err == SOCKET_ERROR ) {
										EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
										closesocket(m_socket);
										WSACleanup();
										return TRUE;
									}

									// connect to server
									connect(m_socket,(SOCKADDR *)&client_sin,sizeof(client_sin));
									EditPrintf(hwndEdit, TEXT("=== connection build with server ===\r\n"));


									// initialize client requested job and maintained
									singleClient* sC;
									sC = globalClientHandler.findClient(*it);
									if(sC == NULL); // no such client
									else{
										//aClientJobs[currentUser].init(sServerIP,sServerPort,sServerFile,currentUser,m_socket);
										//currentUser++;
										sC->init(*it);
										clientJob* cJ;
										cJ = sC->getANewJob();
										cJ->init(sServerIP,sServerPort,sServerFile,sC->currentUserNum - 1,m_socket);
										EditPrintf(hwndEdit, TEXT("=== finish initializing a job===\r\n"));
									}
								}	
							}

							// valid request : send initial web page
							if(isInvalid == false){
								EditPrintf(hwndEdit, TEXT("=== sending initial web page===\r\n"));

								singleClient* sC;
								sC = globalClientHandler.findClient(*it);

								string webpage;
								webpage = string("<html>")
									+ string("<head>")
									+ string("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />")
									+ string("<title>Network Programming Homework 3</title>")
									+ string("</head>")
									+ string("<body bgcolor=#336699>")
									+ string("<font face=\"Courier New\" size=2 color=#FFFF99>")
									+ string("<table width=\"800\" border=\"1\">")
									+ string("<tr>");
								for(int i = 0; i < sC->getUserNum(); i++){
									clientJob* cJ;
									cJ = sC->getAnExisJob(i);
									if(cJ->m_sSeverIP == "") continue;
									else webpage += string("<td>") + string(cJ->m_sSeverIP) + string("</td>");
								}
								webpage += string("</tr><tr>");

								for(int i = 0; i < sC->getUserNum(); i++){
									clientJob* cJ;
									cJ = sC->getAnExisJob(i);
									if(cJ->m_sSeverIP == "") continue;
									else webpage += string("<td valign=\"top\" id=\"m") + string(to_string((unsigned long long)i)) + string("\"></td>");
								}
								string webpage2;
								webpage2 += string("</tr></table>\n");

								send(sC->serverSocketFD, webpage.c_str(), webpage.size(),0);
								send(sC->serverSocketFD, webpage2.c_str(), webpage2.size(),0);
							}
						}

						delete[] recvbuffer;
					}
					
					break;
				case FD_WRITE:
					//Write your code for write event here
					
				case FD_CLOSE:
					break;
			};
			break;
		case WM_SOCKET_NOTIFY_SERVER:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_READ:
					for(int c = 0; c < globalClientHandler.vClients.size(); c++){
						
						singleClient* sC;
						sC = &(globalClientHandler.vClients[c]);
						EditPrintf(hwndEdit, TEXT("=== reading client socket fd: %d ===\r\n"), sC->serverSocketFD);

						for(int i = 0; i < sC->getUserNum(); i++){

							clientJob* cJ;
							cJ = sC->getAnExisJob(i);
							cJ->readUntilEnd();
							//string currentResult = cJ.getResultString();
							//EditPrintf(hwndEdit, TEXT("=== reading content %s ===\r\n"), currentResult.c_str());
							string webpage = cJ->getResultString() + string("\n");
							send(sC->serverSocketFD, webpage.c_str(), webpage.size(),0);
							
							if(cJ->isReadyToWrite && cJ->isFinish) continue; // completed already


							if(cJ->isReadyToWrite) cJ->doAJob();

							Sleep(1000);
							if(cJ->isFinish){
								Sleep(3000);
								cJ->readUntilEnd();
								webpage = cJ->getResultString() + string("\n");
								send(sC->serverSocketFD, webpage.c_str(), webpage.size(),0);
							}else{
								webpage = cJ->getResultString() + string("\n");
								send(sC->serverSocketFD, webpage.c_str(), webpage.size(),0);
								cJ->isReadyToWrite = FALSE;
							}
						}

						//check if to output webpage
						bool isAllReady;
						isAllReady = true;
						for(int i = 0; i < sC->getUserNum(); i++){
							clientJob* cJ;
							cJ = sC->getAnExisJob(i);

							if(((cJ->isFinish == TRUE) && (cJ->isReadyToWrite == TRUE)) == FALSE){
								isAllReady = FALSE;
								break;
							}
						}
						//EditPrintf(hwndEdit, TEXT("===  IS READY TO OUTPUT A WEBPAGE : %d===\r\n"), isAllReady);
						if(isAllReady){
							// combine all results
							EditPrintf(hwndEdit, TEXT("===  IS READY TO OUTPUT A WEBPAGE ==="));

							string webpage2 = "";
							webpage2 += string("</font>")
								+ string("</body>")
								+ string("</html>\n");

							send(sC->serverSocketFD, webpage2.c_str(), webpage2.size(),0);
							closesocket(sC->serverSocketFD);
							globalClientHandler.deleteClient(sC->serverSocketFD);
							c--;
						}
					}
					break;
				case FD_WRITE:
					break;
				case FD_CLOSE:
					break;
			};
			break;
		default:
			return FALSE;


	};

	return TRUE;
}

int EditPrintf (HWND hwndEdit, TCHAR * szFormat, ...)
{
     TCHAR   szBuffer [2048] ;
     va_list pArgList ;

     va_start (pArgList, szFormat) ;
     wvsprintf (szBuffer, szFormat, pArgList) ;
     va_end (pArgList) ;

     SendMessage (hwndEdit, EM_SETSEL, (WPARAM) -1, (LPARAM) -1) ;
     SendMessage (hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) szBuffer) ;
     SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0) ;
	 return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0); 
}