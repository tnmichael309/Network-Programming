#include "proxyServer.h"

static void splitString(string originalString, vector<string>& resultString){

	stringstream ss(originalString);
	string subString;
	while(getline(ss,subString,'&')) // divide string by '&'
		resultString.push_back(subString);	

	return;
}

int server::staticListenerD = 0;

void copyStringToCharArray(string s, char* c){
	int size = s.length()+1 > 1024 ? 1023 : s.length();
	
	for(int i = 0; i < size; i++){
		c[i] = s[i];
	}
	c[size] = '\0';
}

void server::handleShutdown(int sig){
	
	
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

server::server(){;}

server::~server(){;}

void server::start(int port, int constraint)
{
	
	constraintType = constraint;
	
	// create listener socket
	if(catchSignal(SIGINT, server::handleShutdown) == -1)
		error("Can't set the SIGINT handler");
	if(catchSignal(SIGPIPE, server::handleClientClosed) == -1)
		error("Can't set the SIGPIPE pipe handler");
	if(catchSignal(SIGCHLD, SIG_IGN) == -1)
		cerr << "Can't set the SIGCHLD handler" << endl;

	staticListenerD = openListenerSocket();

	bindToPort(staticListenerD, port);
	if(listen(staticListenerD, 30) < 0)
		error("Can't listen");
	

	struct sockaddr_in client_addr; /* the from address of a client*/
	unsigned int address_size = sizeof(client_addr);

	cout << "Start listen" << endl;
	
	while(1){
		int newsockfd = accept(staticListenerD, (struct sockaddr *) &client_addr, (socklen_t*)&address_size);
		//cout << newsockfd << endl;
		
		// no new socket received
		if (newsockfd < 0){ 
			close(newsockfd);
			continue;
		}
		
		
		char clntIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr),clntIP,sizeof(clntIP));
		//cout << "<S_IP>:\t" << clntIP << endl;
		//cout << "<S_Port>:\t" << (int) ntohs(client_addr.sin_port) << endl;
		srcIP = clntIP;
		srcPort = to_string((int) ntohs(client_addr.sin_port));
		
		int childpid;
		if ( (childpid = fork()) < 0) cerr << "server: fork error" << endl;
		else if (childpid == 0) { // child process
			
			// process the request
			// ignore SIGCHLD
			signal(SIGCHLD, NULL);
			
			handleClient(newsockfd);
				
			if(staticListenerD) close(staticListenerD); 
			exit(0);
		}else{// parent process
			// do nothing
			close(newsockfd);
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
	unsigned char SOCKS4_REQUEST[20];
	
	// receive SOCKS4_REQUEST
	while(recv(sockfd, SOCKS4_REQUEST, 20, 0) <= 0);
	
	// return SOCKS4_REPLY
	unsigned char SOCKS4_REPLY[8] = {0};
	SOCKS4_REPLY[0] = 0; // must be 0
	SOCKS4_REPLY[1] = 90; // granted
	SOCKS4_REPLY[2] = SOCKS4_REQUEST[2];
	SOCKS4_REPLY[3] = SOCKS4_REQUEST[3];
	SOCKS4_REPLY[4] = SOCKS4_REQUEST[4];
	SOCKS4_REPLY[5] = SOCKS4_REQUEST[5];
	SOCKS4_REPLY[6] = SOCKS4_REQUEST[6];
	SOCKS4_REPLY[7] = SOCKS4_REQUEST[7];
	
	// process destination port and ip
	string port = to_string(((int)SOCKS4_REQUEST[2]) * 256 + ((int)SOCKS4_REQUEST[3]) );
	string ip = to_string((int)SOCKS4_REQUEST[4]);
	ip += '.';
	ip += to_string((int)SOCKS4_REQUEST[5]);
	ip += '.';
	ip += to_string((int)SOCKS4_REQUEST[6]);
	ip += '.';
	ip += to_string((int)SOCKS4_REQUEST[7]);
	
	
	// check if able to go through firewall
	bool isAllowed = true;
	if(constraintType == 0); // all allowed 
	else if(constraintType == 1){ //NCTU only
		if((int)SOCKS4_REQUEST[4] == 140 && (int)SOCKS4_REQUEST[5] == 113 );
		else isAllowed = false;
	}else{	//NTHU only
		if((int)SOCKS4_REQUEST[4] == 140 && (int)SOCKS4_REQUEST[5] == 114 );
		else isAllowed = false;
	}
	if(!isAllowed){
		cout << "Invalid Connection" << endl;
		// return invalid reply
		SOCKS4_REPLY[1] = 91;
		write(sockfd, SOCKS4_REPLY, 8);
		cout << "<S_IP>:   " << srcIP << endl;
		cout << "<S_Port>: " << srcPort << endl;
		cout << "<D_IP>:   " << ip << endl;
		cout << "<D_Port>: " << port << endl;
		cout << "<Reply>:  " << "Denied" << endl;
	
		return -1;
	}else{
		// show connection message
		cout << "<S_IP>:   " << srcIP << endl;
		cout << "<S_Port>: " << srcPort << endl;
		cout << "<D_IP>:   " << ip << endl;
		cout << "<D_Port>: " << port << endl;
		cout << "<Reply>:  " << "Granted" << endl;
	}
	
	
	// check mode
	//cerr << "Received Data: " << endl;
	for(int i = 0; i < 20; i++){
		//cout << (int)SOCKS4_REQUEST[i] << endl;
	}
	
	
	if((int)SOCKS4_REQUEST[1] == 1){ // connect mode
	
		/*for(int i = 2; i < 8; i++){
			SOCKS4_REPLY[i] = SOCKS4_REQUEST[i];
		}*/
		//write(sockfd, SOCKS4_REPLY, 8);
		
		connectModeHandler(sockfd, ip, port, SOCKS4_REPLY);
	}else{
		
		bindModeHandler(sockfd, ip, port);
	}
	
	return 1;
}

void server::connectModeHandler(int sockfd, string ip, string port, unsigned char* SOCKS4_REPLY){

	int workerfd;
	struct sockaddr_in  worker_sin;
	struct hostent	*he;
	struct in_addr ipv4addr;
	
	if(!inet_aton(ip.c_str(), &ipv4addr)){
		
	}
	
	
	
	he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
	workerfd = socket(AF_INET,SOCK_STREAM,0);
	memset(&worker_sin, 0, sizeof(worker_sin)); 
	worker_sin.sin_family = AF_INET;
	worker_sin.sin_addr = *((struct in_addr *)he->h_addr); 
	worker_sin.sin_port = htons(atoi(port.c_str()));
	
	// setting non-blocking socket
	int flags;
	if (-1 == (flags = fcntl(workerfd, F_GETFL, 0)))
		flags = 0;
	fcntl(workerfd, F_SETFL, flags | O_NONBLOCK);
	
	
	// keep connecting workerfd
	int status;
	while(1){
		if((status=connect(workerfd,(struct sockaddr *)&worker_sin,sizeof(worker_sin))) == -1) {
			
		}else{
			//cerr << "connect success for user#: " << i << "\n";
			break; // connect successfully
		}
		
	}	
	
	
	fd_set	rfds, afds;
	// handle fds
	FD_ZERO(&rfds);
	FD_ZERO(&afds);
	
	
	int maxfd = 0;
	if(workerfd > maxfd) maxfd = workerfd;
	if(sockfd > maxfd) maxfd = sockfd;
	// connect succesfully and set afds
	FD_SET(workerfd, &afds);
	FD_SET(sockfd, &afds);
	
	//cout << "Connection Built, Status: " << status << endl;
	
	int count = 0;

	
	
	write(sockfd, SOCKS4_REPLY, 8);
	
	
	while(1){
	
		memcpy(&rfds, &afds, sizeof(afds));
		if(select(maxfd+1,&rfds,NULL,NULL,NULL) < 0) return;
		
		int bytesReceived = 0;
		
		
		int bufSize = 16384000;
		char buf[bufSize];
		memset(buf, 0, sizeof(buf));
		
		int counter = 0;
		
		if(FD_ISSET(workerfd, &rfds)){	
			if((bytesReceived = recv(workerfd, buf, bufSize,0)) > 0){
				write(sockfd, buf, bytesReceived);
				cout << "<S_IP>:   " << srcIP << endl;
				cout << "<S_Port>: " << srcPort << endl;
				cout << "<D_IP>:   " << ip << endl;
				cout << "<D_Port>: " << port << endl;
				cout << "<Content>:\n"  << string(buf).substr(0,100) << endl;
				memset(buf, 0, sizeof(buf));
			}
			if(bytesReceived <= 0){
				FD_CLR(workerfd,&afds);
				counter++;
			}
		}
		
		if(FD_ISSET(sockfd, &rfds)){
			if((bytesReceived = recv(sockfd, buf, bufSize,0)) > 0){
				write(workerfd, buf, bytesReceived);
				cout << "<S_IP>:   " << srcIP << endl;
				cout << "<S_Port>: " << srcPort << endl;
				cout << "<D_IP>:   " << ip << endl;
				cout << "<D_Port>: " << port << endl;
				cout << "<Content>:\n"  << string(buf).substr(0,100) << endl;
				memset(buf, 0, sizeof(buf));
			}
			if(bytesReceived <= 0){
				FD_CLR(sockfd,&afds);
				counter++;
			}
		}
		
		//cout << counter;
		if(counter == 2) break;
	}
	close(sockfd);
	close(workerfd);
}

void server::bindModeHandler(int sockfd, string ip, string port){

	int listener = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in bind_addr;
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);// bind any ip, any port
	bind_addr.sin_port = htons(INADDR_ANY);
	
	// bind to port
	if(bind(listener, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0){
		cerr << "Bind Error" << endl;
		exit(1);
	}
	
	int z, sa_len;
	struct sockaddr_in sa;
	sa_len = sizeof(sa);
	z = getsockname(listener, (struct sockaddr*)&sa, (socklen_t*)&sa_len);
	if(z == -1) cerr << "get sock name error" << endl;
	else;// cerr << "Socket IP: " << inet_ntoa(sa.sin_addr) << endl;
	
	// and start listening
	if(listen(listener,5) < 0){
		error("Unable to listen");
	}
	
	
	// return reply 
	unsigned char SOCKS4_REPLY[8] = {0};
	SOCKS4_REPLY[0] = 0; // must be 0
	SOCKS4_REPLY[1] = 90; // granted
	SOCKS4_REPLY[2] = (unsigned char)((ntohs(sa.sin_port))/256); // must be 0
	SOCKS4_REPLY[3] = (unsigned char)((ntohs(sa.sin_port))%256); // granted
	SOCKS4_REPLY[4] = 0;
	SOCKS4_REPLY[5] = 0;
	SOCKS4_REPLY[6] = 0;
	SOCKS4_REPLY[7] = 0;
	
	write(sockfd, SOCKS4_REPLY, 8); // first reply
	
	// listen for the server to connect 
	struct sockaddr_in client_addr; /* the from address of a client*/
	unsigned int address_size = sizeof(client_addr);

	//cerr << "Start Accepcting..." << endl;
	int workerfd = accept(listener, (struct sockaddr *) &client_addr, (socklen_t*)&address_size);
	
	//cerr << "Accepted" << endl;
	write(sockfd, SOCKS4_REPLY, 8); // second reply
	
	
	// start communication
	fd_set	rfds, afds;
	// handle fds
	FD_ZERO(&rfds);
	FD_ZERO(&afds);
	
	
	int maxfd = 0;
	if(workerfd > maxfd) maxfd = workerfd;
	if(sockfd > maxfd) maxfd = sockfd;
	// connect succesfully and set afds
	FD_SET(workerfd, &afds);
	FD_SET(sockfd, &afds);
	
	bool isServerEnd = false;
	bool isClientEnd = false;
	while(1){
		FD_ZERO(&rfds);
		memcpy(&rfds, &afds, sizeof(afds));
		if(select(maxfd+1,&rfds,NULL,NULL,NULL) < 0) return;
		
		int bytesReceived = 0;
		
		char buf[4096];
		memset(buf, 0, sizeof(buf));
		
		if(FD_ISSET(workerfd, &rfds)){	
			//cerr << "In worker reading" << endl;
			if((bytesReceived = recv(workerfd, buf, 4096,0)) > 0){
				write(sockfd, buf, bytesReceived);
				cout << "<S_IP>:   " << srcIP << endl;
				cout << "<S_Port>: " << srcPort << endl;
				cout << "<D_IP>:   " << ip << endl;
				cout << "<D_Port>: " << port << endl;
				cout << "<Content>:\n"  << string(buf).substr(0,100) << endl;
				memset(buf, 0, sizeof(buf));
			}
			if(bytesReceived <= 0){
				FD_CLR(workerfd,&afds);
				isServerEnd = true;
				//cerr << "Clear worker fd" << endl;
				break;
			}
		}
		
		if(FD_ISSET(sockfd, &rfds)){
			//cerr << "In client reading" << endl;
			if((bytesReceived = recv(sockfd, buf, 4096,0)) > 0){
				write(workerfd, buf, bytesReceived);
				cout << "<S_IP>:   " << srcIP << endl;
				cout << "<S_Port>: " << srcPort << endl;
				cout << "<D_IP>:   " << ip << endl;
				cout << "<D_Port>: " << port << endl;
				cout << "<Content>:\n"  << string(buf).substr(0,100) << endl;
				memset(buf, 0, sizeof(buf));
			}
			if(bytesReceived <= 0){
				FD_CLR(sockfd,&afds);
				isClientEnd = true;
				break;
				//cerr << "Clear sock fd" << endl;
			}
		}
		
		if(isServerEnd && isClientEnd) break;
	}
	close(sockfd);
	close(workerfd);
	
}


