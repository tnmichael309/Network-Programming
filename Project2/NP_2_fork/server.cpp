#include "server.h"

#define shmKey ((key_t) 789)

Mesg *pMesg;
int shmid;
server* mainServer;

int server::staticListenerD = 0;

void copyStringToCharArray(string s, char* c){
	int size = s.length()+1 > 1024 ? 1023 : s.length();
	
	for(int i = 0; i < size; i++){
		c[i] = s[i];
	}
	c[size] = '\0';
}

void server::handleShutdown(int sig){
	
	// remove share memory at parent	
	//cout << "Share memory removing..." << endl;
	if(!(shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0) < 0)){
		//cout << "Share memory removed" << endl;
	}
	
	if(staticListenerD)
		close(staticListenerD);
	exit(0);
}

void server::handleClientClosed(int sig){
		
	if(staticListenerD)
		close(staticListenerD);
}

void server::sigchldHandler(int sig){
	int status;
	wait(&status);
}

void server::sigusrHandler(int sig){
	
	if(sig == SIGUSR1){
		// sent from child
		////////////////////
		// process asked command
		////////////////////
		// attach the segment to our data space
		//cout << "attaching share memory" << endl;
		if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
			cerr << "shmat" << endl;
			exit(1);
		}

		// if the command need to be processed
		//cout << "find new command here" << endl;

		string message[5];
		for(int i = 0; i < 5; i++){
			message[i] = pMesg->message[i];
		}

		if(message[0] == "who") mainServer->whoServer(pMesg->srcSocketFd);
		else if(message[0] == "name") mainServer->nameServer(pMesg->srcSocketFd, message[1]);
		else if(message[0] == "tell") mainServer->tellServer(pMesg->srcSocketFd, message[1]);
		else if(message[0] == "yell") mainServer->yellServer(pMesg->srcSocketFd, message[1]);
		else if(message[0] == "redirect input output") pMesg->isSuccess = mainServer->redirectToOtherUser(message[1],pMesg->srcSocketFd);
		else if(message[0] == "exit"){
			//cout << "exit sock fd: " << pMesg->srcSocketFd << endl;
			
			int curID = mainServer->m_clientPools.findUserByFD(pMesg->srcSocketFd);
			
			string offlineReminder = "*** User \'";
			string userName = mainServer->m_clientPools.getUserName(curID);
			if(userName == "no name") offlineReminder += "(no name)";
			else offlineReminder += userName;
			offlineReminder += "\' left. ***\n";
			
			
			mainServer->m_fifos.clearConnectedInfo(curID);
			mainServer->broadcastMessageFromServer(0, offlineReminder);	
			mainServer->deleteUser(pMesg->srcSocketFd);
			close(pMesg->srcSocketFd);
			//cout << "In exit" << endl;
		}else if(message[0] == "broadcast"){
			int curID = mainServer->m_clientPools.findUserByFD(pMesg->srcSocketFd);
			
			mainServer->broadcastMessageFromServer(curID, message[1]);
		}
		
		int senderPid = pMesg->srcPid;
		
		// detach share mem
		//cout << "detaching share memory" << endl;
		if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
		
		kill(senderPid, SIGUSR2);
		
		//cout << "server sigusr1 complete" << endl;
	}else{
	
		// do nothing
	}
		
	
}

server::server(){;}

server::~server(){;}

void server::start(int port)
{
	// initial main server
	mainServer = this;
	
	// create share memory segment
	// parent and children should all have this segment now
	if ((shmid = shmget(shmKey, sizeof(Mesg), IPC_CREAT | 0666)) < 0) {
        cerr << "shmget" << endl;
        exit(1);
    }
	
	// create listener socket
	if(catchSignal(SIGINT, server::handleShutdown) == -1)
		error("Can't set the SIGINT handler");
	if(catchSignal(SIGPIPE, server::handleClientClosed) == -1)
		error("Can't set the SIGPIPE pipe handler");
	if(catchSignal(SIGCHLD, SIG_IGN) == -1)
		cerr << "Can't set the SIGCHLD handler" << endl;
	if(catchSignal(SIGUSR1, server::sigusrHandler) == -1)
		cerr << "Can't set the SIGCHLD handler" << endl;
	if(catchSignal(SIGUSR2, server::sigusrHandler) == -1)
		cerr << "Can't set the SIGCHLD handler" << endl;

	
	staticListenerD = openListenerSocket();

	bindToPort(staticListenerD, port);
	if(listen(staticListenerD, 30) < 0)
		error("Can't listen");
	

	struct sockaddr_in client_addr; /* the from address of a client*/
	unsigned int address_size = sizeof(client_addr);

	cout << "Start listen" << endl;
	/*int game_board[16] = {2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0};
	
	int best_move = game_ai_.find_best_move(game_board, 16);
	cout << best_move << endl;
	*/
	while(1){
		int newsockfd = accept(staticListenerD, (struct sockaddr *) &client_addr, (socklen_t*)&address_size);
		//cout << newsockfd << endl;
		
		// no new socket received
		if (newsockfd < 0){ 
			close(newsockfd);
			continue;
		}
		
		// new socket has come
		char clntIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr),clntIP,sizeof(clntIP));
		printf("client IP is %s\n",clntIP);
		printf("client port is %d\n", (int) ntohs(client_addr.sin_port));
		
		// add user to the client pool
		//int curID = m_clientPools.addUser(clntIP, to_string((int)ntohs(client_addr.sin_port)), newsockfd);
		int curID = m_clientPools.addUser("CGILAB", "511", newsockfd);
		m_clientPools.showAllUsers(curID);
		string werlcomeMessage = "****************************************\n** Welcome to the information server. **\n****************************************\n";
		sendMessage(curID, werlcomeMessage);
		
		string onlineReminder = "*** User \'(no name)\' entered from ";
		onlineReminder += "CGILAB";
		onlineReminder += "/";
		onlineReminder += "511";
		onlineReminder += ". ***\n";
		
		broadcastMessageFromServer(curID, onlineReminder);
		sendMessage(curID, "% ");
		
		int childpid;
		if ( (childpid = fork()) < 0) cerr << "server: fork error" << endl;
		else if (childpid == 0) { // child process
			
			// process the request
			// ignore SIGCHLD
			signal(SIGCHLD, NULL);
			
			
			// close other process's fd
			m_clientPools.closeOtherSocketFD(curID);
			handleClient(newsockfd);
			m_fifos.clearConnectedInfo(curID);
			
			
			// the client exit
			
			// attach the segment to our data space
			if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
				cerr << "shmat" << endl;
				exit(1);
			}

			// set the command
			pMesg->srcSocketFd = newsockfd;
			copyStringToCharArray("exit",pMesg->message[0]);
			pMesg->srcPid = getpid();
			//cout << "name to change: " << pMesg->message[1] << endl;

			// detach
			if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
			
			// tell parent to do something
			pid_t ppid = getppid();
			kill(ppid, SIGUSR1);
	
			pause();
			
			if(staticListenerD) close(staticListenerD); 
			
			//cout << "Exist called." << endl;
			exit(0);
		}else{// parent process
			// do nothing
			//close(newsockfd);
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
	int s = socket(AF_INET, SOCK_STREAM, 0);
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
	if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1)
		error("Can't set the reuse option on the socket");
	
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
	
	// set the current sockfd
	currentSocketFD = sockfd;
	
	// get client shell and initialize self setting
	Shell *p = m_clientPools.getClientShell(curID);
	
	while(1){
		// process command
		string tempStr = "";
		std::getline(std::cin,tempStr);

		tempStr += "\n";

		if(tempStr.size() == 1 || tempStr.size() == 2) continue;
		
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
			if(redirectClient(tempStr, curID, readFd, writeFd,readRedirectMessage,writeRedirectMessage,falseMessage) == true){
				p->processCommand(tempStr);
				if(readFd != -1){
					// broadcast
					// attach the segment to our data space
					if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
						cerr << "shmat" << endl;
						exit(1);
					}

					// set the command
					pMesg->srcSocketFd = currentSocketFD;
					copyStringToCharArray("broadcast",pMesg->message[0]);
					copyStringToCharArray(readRedirectMessage,pMesg->message[1]);
					pMesg->srcPid = getpid();
					//cout << "name to change: " << pMesg->message[1] << endl;

					// detach
					if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
					
					// tell parent to do something
					pid_t ppid = getppid();
					kill(ppid, SIGUSR1);
			
					pause();
				}
				if(writeFd != -1){
					// broadcast
					if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
						cerr << "shmat" << endl;
						exit(1);
					}

					// set the command
					pMesg->srcSocketFd = currentSocketFD;
					copyStringToCharArray("broadcast",pMesg->message[0]);
					copyStringToCharArray(writeRedirectMessage,pMesg->message[1]);
					pMesg->srcPid = getpid();
					//cout << "name to change: " << pMesg->message[1] << endl;

					// detach
					if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
					
					// tell parent to do something
					pid_t ppid = getppid();
					kill(ppid, SIGUSR1);
			
					pause();
				}
			}else{
				redirectSuccess = false;
			}
			
			//cerr << "Updated read write fd: " << readFd << " " << writeFd << endl;
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


		//m_clientPools.restoreClientShell(curID);

		if(!redirectSuccess) cout << falseMessage;
		
		if(p->isExit()){
			//server::handleShutdown(0);
			/*dup2(tempStdinFd, 0);
			dup2(tempStdoutFd, 1);
			dup2(tempStderrFd, 2);*/
			//exit(0);
			return 0;
		}
		
		cout << "% " << flush;
	}
	
	cerr << "Should never out of client handler loop here" << endl;
}

void server::deleteUser(int newsockfd){
	int curID = m_clientPools.findUserByFD(newsockfd);
	m_clientPools.delUser(m_clientPools.findUserByFD(newsockfd));
	m_fifos.clearConnectedInfo(curID);
};

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
			whoClient(curID);
			return;
		case 1:
			nameClient(curID, s);
			return;
		case 2:
			tellClient(curID, s);
			return;
		case 3:
			yellClient(curID, s);
			return;
		default:
			return;
	} 
}

void server::whoServer(int srcFD){
	int tempStdoutFd = dup(1);
	dup2(srcFD, 1);
	
	int srcID = m_clientPools.findUserByFD(srcFD);
	m_clientPools.showAllUsers(srcID);
	
	dup2(tempStdoutFd, 1);
}

void server::whoClient(int curID){
	
	// attach the segment to our data space
	if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
		cerr << "shmat" << endl;
		exit(1);
	}
	
	// set the command
	pMesg->srcSocketFd = currentSocketFD;
	copyStringToCharArray("who",pMesg->message[0]);
	pMesg->srcPid = getpid();
	
	// detach
	if (shmdt(pMesg) < 0) cout << "client: can't detach shared memory" << endl;
	
	// tell parent to do something
	pid_t ppid = getppid();
	kill(ppid, SIGUSR1);
	
	pause();
	// wait until SIGUSR2 is delivered 
	//sigsuspend(NULL);
		
	
}

void server::nameServer(int srcFD, string s){ // s: now is the name to change
	
	
	//cout << "name received by server: " << s << endl;
	
	int curID = m_clientPools.findUserByFD(srcFD);
	
	if(m_clientPools.setName(curID, s));
	else{
		int tempStdoutFd = dup(1);
		dup2(srcFD, 1);
		cout << "*** User \'" << s << "\' already exists.***" << endl;
		dup2(tempStdoutFd, 1);
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
	message += s;
	message += "\'. ***\n";
	broadcastMessageFromServer(curID, message);
	
}

void server::nameClient(int curID, string s){

	
	int loc1 = s.find_first_of(' ');
	int loc2 = s.find_first_not_of(' ',loc1);
	
	// remove empty or carriage return
	for(int i = s.length() - 1; i >= 0; i--){
		// there might be a carriage return ? I don't know wtf it is?
		// but I just find out, bitch! carriage return  <- 13
		if(s[i] == ' ' || s[i] == 13 || s[i] == '\n') s.pop_back(); 
		else break;
	}
	
	// attach the segment to our data space
	if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
		cerr << "shmat" << endl;
		exit(1);
	}
	
	// set the command
	pMesg->srcSocketFd = currentSocketFD;
	copyStringToCharArray("name",pMesg->message[0]);
	copyStringToCharArray(s.substr(loc2),pMesg->message[1]);
	pMesg->srcPid = getpid();
	//cout << "name to change: " << pMesg->message[1] << endl;
	
	// detach
	if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
	
	// tell parent to do something
	pid_t ppid = getppid();
	kill(ppid, SIGUSR1);
	
	pause();
	
}

void server::tellServer(int srcFD, string s){
	
	int tempStdoutFd = dup(1);
	int tempStderrFd = dup(2);
	dup2(srcFD, 1);
	dup2(srcFD, 2);
	
	int curID = m_clientPools.findUserByFD(srcFD);
	
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
		dup2(tempStdoutFd, 1);
		dup2(tempStderrFd, 2);
		return ;
	}else;
	string tarID = s.substr(loc1, loc2 - loc1);
	
	int loc3 = s.find_first_not_of(" ",loc2);
	if(loc3 == string::npos){
		cerr << "Invalid use of [tell [user id] [content]]" << endl;
		dup2(tempStdoutFd, 1);
		dup2(tempStderrFd, 2);
		return ;
	}else;
	string content = s.substr(loc3);
	
	if(m_clientPools.isActiveUser(atoi(tarID.c_str()) - 1) == false){
		cout << "*** Error: user #" << tarID << " does not exist yet. ***" << endl;
		dup2(tempStdoutFd, 1);
		dup2(tempStderrFd, 2);
		return;
	}
	
	string finalContent = "*** ";
	finalContent += m_clientPools.getUserName(curID);
	finalContent += " told you ***: ";
	finalContent += content;
	
	sendMessage(curID,atoi(tarID.c_str()) - 1,finalContent);
	dup2(tempStdoutFd, 1);
	dup2(tempStderrFd, 2);
};

void server::tellClient(int curID, string s){
	
	
	// attach the segment to our data space
	if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
		cerr << "shmat" << endl;
		exit(1);
	}
	
	// set the command
	pMesg->srcSocketFd = currentSocketFD;
	copyStringToCharArray("tell",pMesg->message[0]);
	copyStringToCharArray(s,pMesg->message[1]);
	pMesg->srcPid = getpid();
	//cout << "name to change: " << pMesg->message[1] << endl;
	
	// detach
	if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
	
	// tell parent to do something
	pid_t ppid = getppid();
	kill(ppid, SIGUSR1);
	
	pause();
	
}

void server::yellServer(int srcFD, string s){
	
	int tempStdoutFd = dup(1);
	int tempStderrFd = dup(2);
	dup2(srcFD, 1);
	dup2(srcFD, 2);
	
	int curID = m_clientPools.findUserByFD(srcFD);
	
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
		dup2(tempStdoutFd, 1);
		dup2(tempStderrFd, 2);
		return ;
	}else;
	
	string content = s.substr(loc1);
	string finalContent = "*** ";
	finalContent += m_clientPools.getUserName(curID);
	finalContent += " yelled ***: ";
	finalContent += content;
	finalContent += "\n";
	
	dup2(tempStdoutFd, 1);
	dup2(tempStderrFd, 2);
	
	broadcastMessageFromServer(curID, finalContent);
}

void server::yellClient(int curID, string s){
	// attach the segment to our data space
	if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
		cerr << "shmat" << endl;
		exit(1);
	}
	
	// set the command
	pMesg->srcSocketFd = currentSocketFD;
	copyStringToCharArray("yell",pMesg->message[0]);
	copyStringToCharArray(s,pMesg->message[1]);
	pMesg->srcPid = getpid();
	//cout << "name to change: " << pMesg->message[1] << endl;
	
	// detach
	if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
	
	// tell parent to do something
	pid_t ppid = getppid();
	kill(ppid, SIGUSR1);
	
	pause();
	
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

bool server::redirectToOtherUser(string s, int srcFD){
	
	int curID = m_clientPools.findUserByFD(srcFD);
	
	//cerr << "Original string: " << s << endl;
	int tempStdoutFd = dup(1);
	int tempStderrFd = dup(2);
	dup2(srcFD, 1);
	dup2(srcFD, 2);
	
	
	int loc1 = 0;
	int endLoc = s.size();
	int i= 0;
	string redirectMessages[2] = {"", ""};
	string fd[2] = {"-1","-1"};
	string falseMessage = "";
	string readRedirectMessage = "";
	string writeRedirectMessage = "";
	
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
					dup2(tempStdoutFd, 1);
					dup2(tempStderrFd, 2);
					return false;
				}*/
				
				if(s[loc1] == '>'){
					int tempfd;
					//cout << fd << endl;
					if(m_clientPools.isActiveUser(tarID) == false){
						/*cout << "*** Error: user #" << tarID+1 << " does not exist yet. ***" << endl;
						dup2(tempStdoutFd, 1);
						dup2(tempStderrFd, 2);*/
						falseMessage = "*** Error: user #";
						falseMessage += to_string(tarID + 1);
						falseMessage += " does not exist yet. ***\n";
						copyStringToCharArray(falseMessage, pMesg->message[4]);
						dup2(tempStdoutFd, 1);
						dup2(tempStderrFd, 2);
						return false;
					}else if((tempfd=m_fifos.getFifoToWrite(curID, tarID)) == -1){	
						/*cout << "*** Error: the pipe #" << curID+1 << "->#" << tarID+1 << " already exists. ***" << endl;
						dup2(tempStdoutFd, 1);
						dup2(tempStderrFd, 2);*/
						falseMessage = "*** Error: the pipe #";
						falseMessage += to_string(curID + 1);
						falseMessage += "->#";
						falseMessage += to_string(tarID+1);
						falseMessage += " already exists. ***\n";
						copyStringToCharArray(falseMessage, pMesg->message[4]);
						dup2(tempStdoutFd, 1);
						dup2(tempStderrFd, 2);
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
						fd[1] = to_string(tarID);
					}
				}else if(s[loc1] == '<'){
					int tempfd;
					//cout << fd << endl;
					if(m_clientPools.isActiveUser(tarID) == false || (tempfd= m_fifos.getFifoToRead(tarID, curID)) == -1){
						/*cout << "*** Error: the pipe #" << tarID+1 << "->#" << curID+1 << " does not exist yet. ***" << endl;
						dup2(tempStdoutFd, 1);
						dup2(tempStderrFd, 2);*/
						falseMessage = "*** Error: the pipe #";
						falseMessage += to_string(tarID + 1);
						falseMessage += "->#";
						falseMessage += to_string(curID+1);
						falseMessage += " does not exist yet. ***\n";
						copyStringToCharArray(falseMessage, pMesg->message[4]);
						dup2(tempStdoutFd, 1);
						dup2(tempStderrFd, 2);
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
						fd[0] = to_string(tarID);
					}
				}else{
					cout << "Should not be here in redirect" << endl;
				}
				
			}else continue;
		}
		
		//cerr << "In loop: " << loc1 << endl;
	};
	
	dup2(tempStdoutFd, 1);
	dup2(tempStderrFd, 2);
	
	for(int i = 0; i < 2; i++){
		/*if(redirectMessages[i] != ""){
			broadcastMessageFromServer(curID, redirectMessages[i]);
		}*/
		
		copyStringToCharArray(fd[i],pMesg->message[i]);
		copyStringToCharArray(redirectMessages[i],pMesg->message[i+2]);
	}
	
	//cerr << "Revised string: " << s << endl;
	return true;
	
}

bool server::redirectClient(string& s, int curID, int& readFd, int& writeFd, string& readRedirectMessage, string& writeRedirectMessage, string& falseMessage){

	cout << flush;
	
	// attach the segment to our data space
	if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
		cerr << "shmat" << endl;
		exit(1);
	}
	
	// set the command
	pMesg->isSuccess = false;
	pMesg->srcSocketFd = currentSocketFD;
	copyStringToCharArray("redirect input output",pMesg->message[0]);
	copyStringToCharArray(s,pMesg->message[1]);
	pMesg->srcPid = getpid();
	//cout << "name to change: " << pMesg->message[1] << endl;
	
	// detach
	if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
	
	// tell parent to do something
	pid_t ppid = getppid();
	kill(ppid, SIGUSR1);
	
	pause();
	
	// attach the segment to our data space
	if ((pMesg = (Mesg*)shmat(shmid, NULL, 0)) ==  (Mesg*)-1) {
		cerr << "shmat" << endl;
		exit(1);
	}
	
	// copy pMesg's messages
	string message[5];
	for(int i = 0; i < 5; i++){
		message[i] = pMesg->message[i];
		copyStringToCharArray("",pMesg->message[i]);
	}
	bool isSuccess = pMesg->isSuccess;
	pMesg->isSuccess = false;
	
	// detach
	if (shmdt(pMesg) < 0) cout << "server: can't detach shared memory" << endl;
	
	// update redirect or error message
	readRedirectMessage = message[2];
	writeRedirectMessage = message[3];
	falseMessage = message[4];
	
	if(!isSuccess);
	else{
		// analyze message
		if(message[0] != "-1"){
			string fileName = m_fifos.getFifoName(atoi(message[0].c_str()), curID);
			
			int fd = open(fileName.c_str(), O_RDONLY, S_IRUSR | S_IWUSR);
			
			//cout << "File name: " << fileName << endl;
			//cout << "open read fd: " << fd << endl;
			readFd = fd;
			dup2(fd,0);
		}
		if(message[1] != "-1"){
			
			string fileName = m_fifos.getFifoName(curID, atoi(message[1].c_str()));
			int fd = open(fileName.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
			
			//cout << "File name: " << fileName << endl;
			//cout << "open write fd: " << fd << endl;
			writeFd = fd;
			dup2(fd,1);
		}
		
		
		
		s = reviseString(s);
	}
	
	return isSuccess;
};

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
