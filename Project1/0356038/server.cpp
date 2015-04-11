#include "server.h"
#include "shell.h"

int server::currentServerSocketFd = 0;

server::server(int portNum){

	int sockfd, newsockfd, clilen, childpid;
	struct sockaddr_in cli_addr, serv_addr;
	
	// open a TCP socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		cerr << "Server: Cannot open stream socket" << endl;
	}
	
	// register event handler for ctrl+C at server
	// or SIGPIPE of forked server process
	// close the server!
	currentServerSocketFd = sockfd;
	if(catchSignal(SIGINT, server::serverCloseHandler) == -1)
		cerr << "Can't set the interrupt handler" << endl;
	if(catchSignal(SIGPIPE, server::clientCloseHandler) == -1)
		cerr << "Can't set the interrupt handler" << endl;
	
	//if(catchSignal(SIGCHLD, server::sigchldHandler) == -1)
	//	cerr << "Can't set the child signal handler" << endl;
	struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = &(server::sigchldHandler);
    sigaction(SIGCHLD, &sa, NULL); 
	
	// bind our address so that the client can send message to us 
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portNum);
	
	// telnet nplinux3.cs.nctu.edu.tw 42756
	
	if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		cerr << "Server: Cannot bind local address" << endl;
		
	listen(sockfd, MAX_CONNECTION_NUM);
	
	for ( ; ; ) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen);
		if (newsockfd < 0){
			//cerr << "server: accept error" << endl;
			close(newsockfd);
			continue;
		}
		
		if ( (childpid = fork()) < 0) cerr << "server: fork error" << endl;
		else if (childpid == 0) { 
			close(sockfd); 
			// process the request
			// ignore SIGCHLD
			signal(SIGCHLD, NULL);
			clientHandler(newsockfd);
			exit(0);
		}else{
			close(newsockfd); // parent process
		}
	}
}


server::~server(){;}

void server::clientHandler(int sockfd){

	int tempStdinFd = dup(0);
	int tempStdoutFd = dup(1);
	int tempStderrFd = dup(2);
	dup2(sockfd, 0);
	dup2(sockfd, 1);
	dup2(sockfd, 2);
	
	cout << "****************************************" << endl;
	cout << "** Welcome to the information server. **" << endl;
	cout << "****************************************" << endl;

	cout << "% " << flush;
	/*string exitString = "exit";
	exitString += (char)13;
	*/
	
	Shell p;
	/*if(chdir("~/ras_bsd/") != 0){
		cerr << "Change working directory failed" << endl;
		exit(0);
	}*/
	
	p.setENV("setenv PATH bin:. ");
	
	for ( ; ; ) {	
		string tempStr;
		std::getline(std::cin,tempStr);
		
		/*if(tempStr == exitString){
			server::serverCloseHandler(0);
		}*/
		
		tempStr += "\n";
		p.processCommand(tempStr);

		if(p.isExit()){
			//cout << p.isExit() << endl;
			server::serverCloseHandler(0);
		}
		
		cout << "% " << flush;
	}
	
	dup2(tempStdinFd, 0);
	dup2(tempStdoutFd, 1);
	dup2(tempStderrFd, 2);
	close(tempStdinFd);
	close(tempStdoutFd);
	close(tempStderrFd);
}

int server::catchSignal(int sig, void (*handler)(int))
{
	struct sigaction action;
	action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	return sigaction(sig, &action, NULL);
}

void server::serverCloseHandler(int signal)
{
	
	if(currentServerSocketFd){
		close(currentServerSocketFd);
		//cerr << "\nExit Server!\n";
	}

	exit(0);
}

void server::clientCloseHandler(int signal)
{
	if(currentServerSocketFd)
		close(currentServerSocketFd);
	
	exit(0);
}

void server::sigchldHandler(int signal){
	int status;
	wait(&status);
	//cout << "Finish waiting child" << endl;
}
