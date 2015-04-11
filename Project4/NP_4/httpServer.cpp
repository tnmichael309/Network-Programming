#include "httpServer.h"

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

void server::start(int port)
{
	
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
			
		int childpid;
		if ( (childpid = fork()) < 0) cerr << "server: fork error" << endl;
		else if (childpid == 0) { // child process
			
			// process the request
			// ignore SIGCHLD
			signal(SIGCHLD, NULL);
			
			cout << "connected fd: " << newsockfd << endl;
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
	int tempStdinFd = dup(0);
	int tempStdoutFd = dup(1);
	dup2(sockfd, 0);
	dup2(sockfd, 1);
	
	char buf[1024];
	
	// deal with request
	string receiveMessage;
	string requestType;
	string queryString;
	if(recv(sockfd,buf,1024,0) > 0){
		receiveMessage = string(buf);
		stringstream ss(receiveMessage);
		string subString;
		if(getline(ss,subString,'\n')) receiveMessage = subString;	
		
		//cerr << receiveMessage << endl;
		
		stringstream sss(receiveMessage);
		if(getline(sss,subString,'/') && getline(sss,subString,' ')) requestType = subString;
		
		//cerr << requestType << endl;
		
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
		}else{
			goto finish;
		}
		
		//cout << requestType << "\t" << queryString << endl;
	}else goto finish;
	
	if(requestType.find(".htm")!=string::npos){ // html request
	
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
		cout << webpage << endl;
	}else if(requestType.find("cgi")!=string::npos){ // cgi
		
		//cout << "<html>" << receiveMessage << "<br>" << requestType << "<br>" << queryString << "</html>" << endl;
		
		if(chdir("./CGI") != 0){
			cerr << "Change working directory failed" << endl;
			exit(0);
		}
		setenv("PATH","CGI:.",1);
		setenv("QUERY_STRING", queryString.c_str(),1);
		int returnStatus = execvp(requestType.c_str(),NULL);
		if(returnStatus == -1) cerr << "Unknown command: [" << "cgi" << "]." << endl;
		
	}else;

finish:	
	dup2(tempStdinFd, 0);
	dup2(tempStdoutFd, 1);
	close(tempStdinFd);
	close(tempStdoutFd);
}
