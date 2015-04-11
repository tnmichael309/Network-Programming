#include "server.h"

int server::staticListenerD = 0;

void server::handleShutdown(int sig){
	if(staticListenerD)
		close(staticListenerD);
	exit(0);
}

void server::handleClientClosed(int sig){
	if(staticListenerD)
		close(staticListenerD);
}

server::server(){;}

server::~server(){;}

void server::start(int port)
{
	fd_set rfds; /* read file descriptor set */
	fd_set afds; /* active file descriptor set */
	int fd, nfds;
	
	// create listener socket
	if(catchSignal(SIGINT, server::handleShutdown) == -1)
		error("Can't set the interrupt handler");
	if(catchSignal(SIGPIPE, server::handleClientClosed) == -1)
		error("Can't set the interrupt handler");
	staticListenerD = openListenerSocket();

	bindToPort(staticListenerD, port);
	if(listen(staticListenerD, 200) < 0)
		error("Can't listen");
	
	nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(staticListenerD, &afds);
	
	struct sockaddr_in client_addr; /* the from address of a client*/
	unsigned int address_size = sizeof(client_addr);

	cout << "Start listen" << endl;
	/*int game_board[16] = {2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0};
	
	int best_move = game_ai_.find_best_move(game_board, 16);
	cout << best_move << endl;
	*/
	while(true) {
		memcpy(&rfds, &afds, sizeof(rfds));
		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)
			error("select error");
		if (FD_ISSET(staticListenerD, &rfds)) {
			int ssock;
			ssock = accept(staticListenerD, (struct sockaddr *)&client_addr, &address_size);
			if (ssock < 0)
				error("accept error");
			else{
				char clntIP[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(client_addr.sin_addr),clntIP,sizeof(clntIP));
				printf("client IP is %s\n",clntIP);
				printf("client port is %d\n", (int) ntohs(client_addr.sin_port));
				
				// add user to the client pool
				int curID = m_clientPools.addUser(clntIP, to_string((int)ntohs(client_addr.sin_port)), ssock);
				//m_clientPools.showAllUsers(curID);
				string werlcomeMessage = "****************************************\n** Welcome to the information server. **\n****************************************\n";
				sendMessage(curID, werlcomeMessage);
				
				string onlineReminder = "*** User \'(no name)\' entered from ";
				onlineReminder += string(clntIP);
				onlineReminder += "/";
				onlineReminder += to_string((int)ntohs(client_addr.sin_port));
				onlineReminder += ". ***\n";
				
				broadcastMessageFromServer(curID, onlineReminder);
				sendMessage(curID, "% ");
				
				FD_SET(ssock, &afds);
			}
		}
		for (fd = 0;fd < nfds;fd++) {
			if (fd != staticListenerD && FD_ISSET(fd, &rfds)) {
				if(-1 == handleClient(fd)){
					
					
					string offlineReminder = "*** User \'";
					int curID = m_clientPools.findUserByFD(fd);
					string userName = m_clientPools.getUserName(curID);
					if(userName == "no name") offlineReminder += "(no name)";
					else offlineReminder += userName;
					offlineReminder += "\' left. ***\n";
					
					broadcastMessageFromServer(curID, offlineReminder);	
					
					//m_clientPools.showAllUsers(curID);
					
					m_clientPools.delUser(m_clientPools.findUserByFD(fd));
					m_fifos.clearConnectedInfo(curID);
					
					
					close(fd);
					FD_CLR(fd, &afds); 
					
					
				}
			}
		}
	}
}

void server::error(const char *msg)
{
	cerr << msg << ": " << strerror(errno) << endl;
	exit(1);
}

int server::openListenerSocket()
{
	struct protoent *ppe;
	if ((ppe = getprotobyname("tcp")) == 0) cerr << "can't get protocal entry" << endl;
	
	int s = socket(PF_INET, SOCK_STREAM, ppe->p_proto);
	if(s == -1)
		error("Can't open socket");
	return s;
}

void server::bindToPort(int socket, int port)
{
	struct sockaddr_in name;
	name.sin_family = AF_INET;
	name.sin_port = (in_port_t)htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	int reuse = 1;
	/*if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1)
		error("Can't set the reuse option on the socket");*/
	int c = bind(socket, (struct sockaddr *)&name, sizeof(name));
	if(c == -1)
		error("Can't bind to socket");
}

int server::catchSignal(int sig, void (*handler)(int))
{
	struct sigaction action;
	action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	return sigaction(sig, &action, NULL);
}

int server::handleClient(int sockfd)
{
	int tempStdinFd = dup(0);
	int tempStdoutFd = dup(1);
	int tempStderrFd = dup(2);
	dup2(sockfd, 0);
	dup2(sockfd, 1);
	dup2(sockfd, 2);

	int curID = m_clientPools.findUserByFD(sockfd);
	
	// get client shell and initialize self setting
	Shell* p = (m_clientPools.getClientShell(curID));
	
	// process command
	string tempStr;
	std::getline(std::cin,tempStr);
	
	
	tempStr += "\n";
	/*if(tempStr.size() == 1 || tempStr.size() == 2){
		cout << "%  " << flush;
		dup2(tempStdinFd, 0);
		dup2(tempStdoutFd, 1);
		dup2(tempStderrFd, 2);
		close(tempStdinFd);
		close(tempStdoutFd);
		close(tempStderrFd);
		return 0;
	}
	*/
	int IPCCommand = isIPCCommand(tempStr);
	bool redirectSuccess = true;
	int readFd = -1;
	int writeFd = -1;
	string readRedirectMessage = "";
	string writeRedirectMessage = "";
	string falseMessage = "";
	
	if(IPCCommand != -1){
		exeuteIPCCommand(IPCCommand, tempStr, curID);
	}else{
		// check command and redirect input or output to other user
		if(redirectToOtherUser(tempStr, curID, readFd, writeFd,readRedirectMessage,writeRedirectMessage,falseMessage) == true){
			p->processCommand(tempStr);
			/*close(0);
			close(1);*/
			if(readFd != -1){
				broadcastMessageFromServer(curID, readRedirectMessage);
			}
			if(writeFd != -1){
				broadcastMessageFromServer(curID, writeRedirectMessage);
			}
		}else{
			redirectSuccess = false;
		}
		
		if(readFd != -1){
			close(readFd);
		}
		if(writeFd != -1){
			close(writeFd);
		}
	}
	
	// direct back to current input output
	cout << flush;
	dup2(sockfd, 0);
	dup2(sockfd, 1);
	
	if(!redirectSuccess) cout << falseMessage;
	
	if(!p->isExit()){
		cout << "% " << flush;
		m_clientPools.restoreClientShell(curID);
	}
	
	dup2(tempStdinFd, 0);
	dup2(tempStdoutFd, 1);
	dup2(tempStderrFd, 2);
	close(tempStdinFd);
	close(tempStdoutFd);
	close(tempStderrFd);
	
	
	if(p->isExit()){
		return -1;
	}
	
	return 0;
}

int server::isIPCCommand(string s){

	int loc1 = s.find_first_not_of(' ',0);
	int loc2 = s.find_first_of(" \n",loc1);
	
	string command = s.substr(loc1, loc2 - loc1);
	
	// remove empty or carriage return
	for(int i = command.length() - 1; i >= 0; i--){
		// there might be a carriage return ? I don't know wtf it is?
		// but I just find out, bitch! carriage return  <- 13
		if(command[i] == ' ' || command[i] == 13) command.pop_back(); 
		else break;
	}
	
	if(command == "who") return 0;
	else if(command == "name") return 1;
	else if(command == "tell") return 2;
	else if(command == "yell") return 3;
	else return -1;
	
}

void server::exeuteIPCCommand(int commandSelect, string s, int curID){
	switch(commandSelect){
		case 0:
			who(curID);
			return;
		case 1:
			name(curID, s);
			return;
		case 2:
			tell(curID, s);
			return;
		case 3:
			yell(curID, s);
			return;
		default:
			return;
	} 
}

void server::who(int curID){
	m_clientPools.showAllUsers(curID);
}

void server::name(int curID, string s){
	int loc1 = s.find_first_of(' ');
	int loc2 = s.find_first_not_of(' ',loc1);
	
	// remove empty or carriage return
	for(int i = s.length() - 1; i >= 0; i--){
		// there might be a carriage return ? I don't know wtf it is?
		// but I just find out, bitch! carriage return  <- 13
		if(s[i] == ' ' || s[i] == 13 || s[i] == '\n') s.pop_back(); 
		else break;
	}
	
	if(m_clientPools.setName(curID, s.substr(loc2)));
	else{
		cout << "*** User \'" << s.substr(loc2) << "\' already exists. ***" << endl;
		return;
	}
	
	// successfully set the name
	string ip = m_clientPools.getUserIP(curID);
	string port = m_clientPools.getUserPort(curID);
	string message = "*** User from ";
	message += ip;
	message += "/";
	message += port;
	message += " is named \'";
	message += s.substr(loc2);
	message += "\'. ***";
	broadcastMessage(curID, message);
	
}

void server::tell(int curID, string s){
	// remove empty or carriage return
	for(int i = s.length() - 1; i >= 0; i--){
		// there might be a carriage return ? I don't know wtf it is?
		// but I just find out, bitch! carriage return  <- 13
		if(s[i] == ' ' || s[i] == 13 || s[i] == '\n') s.pop_back(); 
		else break;
	}
	
	int loc = s.find("tell");
	s = s.substr(loc + 5);
	int loc1 = s.find_first_not_of(" ",loc);
	int loc2 = s.find_first_of(" ",loc1);
	if(loc1 == string::npos){
		cerr << "Invalid use of [tell [user id] [content]]" << endl;
		return ;
	}else;
	string tarID = s.substr(loc1, loc2 - loc1);
	
	int loc3 = s.find_first_not_of(" ",loc2);
	if(loc3 == string::npos){
		cerr << "Invalid use of [tell [user id] [content]]" << endl;
		return ;
	}else;
	string content = s.substr(loc3);
	
	if(m_clientPools.isActiveUser(atoi(tarID.c_str()) - 1) == false){
		cout << "*** Error: user #" << tarID << " does not exist yet. ***" << endl;
		return;
	}
	
	string finalContent = "*** ";
	finalContent += m_clientPools.getUserName(curID);
	finalContent += " told you ***: ";
	finalContent += content;
	
	sendMessage(curID,atoi(tarID.c_str()) - 1,finalContent);
}

void server::yell(int curID, string s){

	// remove empty or carriage return
	for(int i = s.length() - 1; i >= 0; i--){
		// there might be a carriage return ? I don't know wtf it is?
		// but I just find out, bitch! carriage return  <- 13
		if(s[i] == ' ' || s[i] == 13 || s[i] == '\n') s.pop_back(); 
		else break;
	}
	
	int loc = s.find("yell");
	s = s.substr(loc + 5);
	int loc1 = s.find_first_not_of(" ",loc);
	if(loc1 == string::npos){
		cerr << "Invalid use of [yell [content]]" << endl;
		return ;
	}else;
	
	string content = s.substr(loc1);
	string finalContent = "*** ";
	finalContent += m_clientPools.getUserName(curID);
	finalContent += " yelled ***: ";
	finalContent += content;
	
	broadcastMessage(curID, finalContent);
}

// client to client
void server::sendMessage(int curID, int tarID, string s){
	int curFD = m_clientPools.findUserFD(curID);
	int targetFD = m_clientPools.findUserFD(tarID);	
	cout << flush;	
	dup2(targetFD, 1);	
	cout << s << endl;	
	dup2(curFD, 1);
}

// server to client
void server::sendMessage(int tarID, string s){
	int tempStdoutFd = dup(1);
	int targetFD = m_clientPools.findUserFD(tarID);
	cout << flush;
	dup2(targetFD, 1);
	cout << s << flush;
	dup2(tempStdoutFd, 1);
	close(tempStdoutFd);
}

// client to client
void server::broadcastMessage(int curID, string s){
	int containerSize = m_clientPools.getContainerSize();
	for(int i = 0; i < containerSize; i++){
		if(m_clientPools.isActiveUser(i)){
			sendMessage(curID, i, s);
		}
	}
}

// server to client
void server::broadcastMessageFromServer(int curID, string s){
	int containerSize = m_clientPools.getContainerSize();
	for(int i = 0; i < containerSize; i++){
		if(m_clientPools.isActiveUser(i)){
			sendMessage(i, s);
		}
	}
}

bool server::redirectToOtherUser(string& s, int curID, int& readFD, int& writeFD, string& readRedirectMessage, string& writeRedirectMessage, string& falseMessage){
	
	//cerr << "Original string: " << s << endl;
	
	int loc1 = 0;
	int endLoc = s.size();
	int i= 0;
	string redirectMessages[2] = {"", ""};
	int fd[2] = {-1, -1};
	
	while(1){
		loc1 = s.find_first_of("><",loc1 +1);
		if(loc1 == string::npos){
			break;
		}else{
			// ">< + number"
			if(s[loc1+1] != ' '){
				
				if(endLoc == s.size()){
					endLoc = loc1 - 1;
					//cout << endLoc << endl;
				}
				
				int loc2 = s.find_first_of(" \n",loc1+1);
				int tarID = atoi( (s.substr(loc1+1, loc2 - (loc1 + 1))).c_str() ) - 1;
				/*if(m_clientPools.isActiveUser(tarID) == false){
					cout << "*** Error: user #" << tarID+1 << " does not exist yet. ***" << endl;
					return false;
				}*/
				
				if(s[loc1] == '>'){
					int tempfd; 
					//cout << fd << endl;
					if(m_clientPools.isActiveUser(tarID) == false){
						//cout << "*** Error: user #" << tarID+1 << " does not exist yet. ***" << endl;
						falseMessage = "*** Error: user #";
						falseMessage += to_string(tarID + 1);
						falseMessage += " does not exist yet. ***\n";
						return false;
					}else if((tempfd= m_fifos.getFifoToWrite(curID, tarID)) == -1){	
						//cout << "*** Error: the pipe #" << curID+1 << "->#" << tarID+1 << " already exists. ***" << endl;;
						falseMessage = "*** Error: the pipe #";
						falseMessage += to_string(curID + 1);
						falseMessage += "->#";
						falseMessage += to_string(tarID+1);
						falseMessage += " already exists. ***\n";
						return false;
					}else{
						string message = "*** ";
						
						string userName = m_clientPools.getUserName(curID);
						
						if(userName == "no name") message += "(no name)";
						else message += userName;
						
						message += " (#";
						message += to_string(curID+1);
						message += ") just piped \'";
						
						string tempStr = s;
						tempStr.pop_back();
						if(tempStr[tempStr.size() - 1] == 13) tempStr.pop_back();
						
						message += tempStr;
						message += "\' to ";
						
						userName = m_clientPools.getUserName(tarID);
						
						if(userName == "no name") message += "(no name)";
						else message += userName;
						
						message += " (#";
						message += to_string(tarID + 1);
						message += ") ***\n";
						
						redirectMessages[1] = message;
						fd[1] = tarID;
					}
				}else if(s[loc1] == '<'){
					int tempfd;
					//cout << fd << endl;
					if(m_clientPools.isActiveUser(tarID) == false || (tempfd= m_fifos.getFifoToRead(tarID, curID)) == -1){
						//cout << "*** Error: the pipe #" << tarID+1 << "->#" << curID+1 << " does not exist yet. ***" << endl;
						falseMessage = "*** Error: the pipe #";
						falseMessage += to_string(tarID + 1);
						falseMessage += "->#";
						falseMessage += to_string(curID+1);
						falseMessage += " does not exist yet. ***\n";
						
						return false;
					}else{
					
						string message = "*** ";
						
						string userName = m_clientPools.getUserName(curID);
						
						if(userName == "no name") message += "(no name)";
						else message += userName;
						
						message += " (#";
						message += to_string(curID+1);
						message += ") just received from ";
						
						userName = m_clientPools.getUserName(tarID);
						
						if(userName == "no name") message += "(no name)";
						else message += userName;
						
						message += " (#";
						message += to_string(tarID + 1);
						message += ") by ";
						message += "\'";
						
						string tempStr = s;
						tempStr.pop_back();
						if(tempStr[tempStr.size() - 1] == 13) tempStr.pop_back();
						
						message += tempStr;
						message += "\' ***\n";
						
						redirectMessages[0] = message;
						fd[0] = tarID;
					}
				}else{
					cout << "Should not be here in redirect" << endl;
				}
				
			}else continue;
		}
		
		//cerr << "In loop: " << loc1 << endl;
	};
	
	/*for(int i = 0; i < 2; i++){
		if(redirectMessages[i] != ""){
			broadcastMessageFromServer(curID, redirectMessages[i]+"\n");
		}
	}*/
	
	if(fd[0] != -1){
		string fileName = m_fifos.getFifoName(fd[0], curID);
		
		int tempfd = open(fileName.c_str(), O_RDONLY, S_IRUSR | S_IWUSR);
		
		//cout << "File name: " << fileName << endl;
		//cout << "open read fd: " << fd << endl;
		readFD = tempfd;
		dup2(tempfd,0);
		
		readRedirectMessage = redirectMessages[0];
	}
	if(fd[1] != -1){
		
		string fileName = m_fifos.getFifoName(curID, fd[1]);
		int tempfd = open(fileName.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
		
		//cout << "File name: " << fileName << endl;
		//cout << "open write fd: " << fd << endl;
		writeFD = tempfd;
		dup2(tempfd,1);
		
		writeRedirectMessage = redirectMessages[1];
	}
	
	
	s = reviseString(s);
	
	
	//cerr << "Revised string: " << s << endl;
	return true;
	
}

string server::reviseString(string s){
	int lastLoc = 0;
	int loc = 0;
	
	string resultString;
	bool isLastASpecialCharacter = false;
	for(loc = 0; loc < s.size(); loc++){
		if(!isLastASpecialCharacter && s[loc] != '<' && s[loc] != '>'){
			resultString += s[loc];
		}else if( (s[loc] == '<' || s[loc] == '>') && s[loc + 1] != ' ' && s[loc + 1] != '\n'){
			isLastASpecialCharacter = true;
		}else if( (s[loc] == '<' || s[loc] == '>')){
			resultString += s[loc];
		}else if(s[loc] == ' ' || s[loc] == '\n'){
			resultString += s[loc];
			isLastASpecialCharacter = false;
		}
	}
	
	//cerr << resultString << endl;
	return resultString;
}

