#include "shell.h"
#include <cstddef>
#include <vector>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <fstream>

static void sigpipe_handler(int status){
	cerr << "SIGPIPE!" << endl;
	exit(1);
};

static void sigchld_handler(int status){
	/* Wait for all dead processes.
	 * We use a non-blocking call to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program. */
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	
	}
	
};

void Shell::processCommand(string s){
	//m_PipePool.showPipeStatus();
	
	//cout << "command: " << s << endl;
	isExitStatus = false;
	
	s = s.substr(s.find_first_not_of(" "));
	
	if(s.find_first_of('/') != string::npos){
		cerr << "Invalid Input with character \'/\'" << endl;
		return;
	}
	
	vector<string> commands;
	size_t foundPos = 0;
	
	size_t lastFoundPos = 0;
	string command = "";
	
	while(true){		
		foundPos = s.find_first_of("\n|", foundPos + 1);
		if(foundPos != string::npos);
		else break;
		
		string subs = s.substr(lastFoundPos, foundPos - lastFoundPos);
		command += subs;
		
		/*
		cout << foundPos << " " << lastFoundPos << endl;
		cout << command << endl;
		cin.get();
		*/
		
		if(s[foundPos] == '\n' || s[foundPos] == '|'){
		
			if(s[foundPos] == '|'){
				string tempString = s.substr(foundPos, s.find_first_of(" \n", foundPos) - foundPos);
				command += tempString;
				foundPos = s.find_first_of(" \n", foundPos) + 1;
			}
			commands.push_back(command);
			command = "";
			
			lastFoundPos = s.find_first_not_of(" ",foundPos);
			
			if(s[foundPos] == '\n') break;
		}else{
			lastFoundPos = foundPos;
		}
	}
	
	//cout << "Splitted commands size: " << commands.size() << endl;
	for(int i = 0; i < commands.size(); i++){
		//cout << commands[i] << endl;
		if(commands[i].find("exit") != string::npos){
			isExitStatus = true;
			return;
		}
		
		bool isSuccesfullyExecuted = executeCommand(commands[i]);
		if(!isSuccesfullyExecuted){
			//cout << "Early breakout" << endl;
			break;
		}
	}
}


bool Shell::executeCommand(string s){

	//cout << "Executing command: " << s << endl;	
	for(int i = s.length() - 1; i >= 0; i--){
		// there might be a carriage return ? I don't know wtf it is?
		// but I just find out, bitch! carriage return  <- 13
		if(s[i] == ' ' || s[i] == 13) s.pop_back(); 
		else break;
	}
	
	// store the current pipe status
	m_PipePool.updateLastState();
	
	int pipefd[2];
	
	// get readPipe fd for the new process
	Pipe readPipe;
	
	if(m_PipePool.getPipeToRead(readPipe)){
		pipefd[READFD] = readPipe.readFD;
	}else{
		pipefd[READFD] = 0;
	}
	
	
	// get writePipe fd for the new process
	size_t foundPos = 0;
	
	if((foundPos=s.find(">>")) != string::npos){
		foundPos=s.find_first_not_of(' ', foundPos + 2);
		string pipeFile = s.substr(foundPos, s.find_first_of(" \n", foundPos+1) - (foundPos));
		//cout << pipeFile << endl;
		//cout << pipeFile.length() << endl;
		pipefd[WRITEFD] = open(pipeFile.c_str(), O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		//cout << pipefd[WRITEFD] << endl;
	}else if((foundPos=s.find_first_of('>')) != string::npos){
		foundPos=s.find_first_not_of(' ', foundPos + 1);
		string pipeFile = s.substr(foundPos, s.find_first_of(" \n", foundPos+1) - (foundPos));
		//cout << pipeFile << endl;
		//cout << pipeFile.length() << endl;
		pipefd[WRITEFD] = open(pipeFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		//cout << pipefd[WRITEFD] << endl;
	}else if((foundPos=s.find_first_of('|')) != string::npos){
		string pipeNum = s.substr(foundPos+1, s.find_first_of(" \n", foundPos+1) - (foundPos+1));
		if(pipeNum == "") pipeNum = "1";
		//cout << pipeNum << endl;
		int iPipeNum = atoi(pipeNum.c_str());
		
		Pipe writePipe = m_PipePool.getPipeToWrite(iPipeNum);
		pipefd[WRITEFD] = writePipe.writeFD;
		
		//m_PipePool.updatePipeCountDown();
	}else{
		// use original stdout
		pipefd[WRITEFD] = 1;
	}
	
	m_PipePool.updatePipeCountDown();
	return createProcess(s,readPipe, pipefd);
}

bool Shell::createProcess(string s, Pipe readPipeToDestroy, int fd[2]){

	// get the command before redirecting : pipe or output to a file
	int loc1 = s.find_first_not_of(" ");
	int loc2 = s.find_first_of("|\n>");
	s = s.substr(loc1,loc2 - loc1);
	
	// remove empty or carriage return
	for(int i = s.length() - 1; i >= 0; i--){
		// there might be a carriage return ? I don't know wtf it is?
		// but I just find out, bitch! carriage return  <- 13
		if(s[i] == ' ' || s[i] == 13) s.pop_back(); 
		else break;
	}
	
	s += "\n";
	
	
		
	vector<string> args;
	size_t foundPos = 0; 
	size_t lastFoundPos = 0;
	
	while(true){
		foundPos = s.find_first_of(" \n", lastFoundPos + 1);
		if(foundPos != string::npos);
		else break;
		
		string subs = s.substr(lastFoundPos, foundPos - lastFoundPos);
		args.push_back(subs);
		
		lastFoundPos = s.find_first_not_of(" ",foundPos);
	}
	
	
	char** argv = new char*[args.size()+1];
	for(int i = 0; i < args.size(); i++){
		argv[i] = new char[args[i].length()+1];
		
		for(int j = 0 ; j < args[i].length(); j++){
			argv[i][j] = args[i][j];
		}
		argv[i][args[i].length()] = '\0';
		
	}
	argv[args.size()] = NULL;
	
	// printenv and setenv
	int externalCommandNum = isSupportedExternalCommand(args[0]);
	//cerr << externalCommandNum << args[0] << " " << s << endl;
	if(externalCommandNum != -1){
		return executeExternalCommand(externalCommandNum, s);
	}else;
	
	// deal with signals
	// 1. SIGPIPE
	signal(SIGPIPE, sigpipe_handler);
	
	if(fd[READFD] == 0); // standard input
	else{ // there is pipe to destroy : destroy write end first, no need to write anymore
		// preserve for fail
		close(readPipeToDestroy.writeFD);
	}
			
	// fork a child process
	pid_t childpid;
	childpid = fork();
	
	if (childpid < 0){
        cerr << "Fork failed" << endl;
        exit(EXIT_FAILURE);
    }
    else if (childpid == 0){ //child process
		
        if (dup2(fd[READFD], 0) != 0){
            cerr << "Child: failed to set up standard input\n";
            exit(EXIT_FAILURE);
        }
        if (dup2(fd[WRITEFD], 1) != 1){
            cerr << "Child: failed to set up standard output\n";
            exit(EXIT_FAILURE);
        }
		
		if(fd[READFD] == 0);
		else{	
			close(fd[READFD]);
		}
		if(fd[WRITEFD] == 1);
		else{
			close(fd[WRITEFD]);
		}
		
		//cerr << args[0] << endl;
		
        int returnStatus = execvp(args[0].c_str(), argv);
		
		if(returnStatus == -1) cerr << "Unknown command: [" << args[0] << "]." << endl;
		
        exit(1);
    }
    else{ // parent process

		int	childState;
				
		
		// wait child to finish
		/*int pid;
		while((childpid != (pid = wait(&childState)))){
			if(pid == -1){
				cerr << "wait error" << endl;
				return false;
			}
		}*/
		if(waitpid(childpid, &childState, 0) == -1)
			cerr << "waitpid error" << endl;
		
		
		//cout << childState << endl;
		if (childState != 0) { 
			// not exist normally
			// 1. signal
			// 2. not exit normally
			// 3. exit with unknown command
			//cout << "recover pool" << endl;
			
			m_PipePool.recover(fd[READFD]);
			//m_PipePool.showPipeStatus();
			
			return false;
		}else{

			// then close the readPipeToDestroy's readFD
			if(fd[READFD] == 0); // standard input
			else{
				close(readPipeToDestroy.readFD);
			}
			/*if(fd[WRITEFD] == 1);
			else{
				close(fd[WRITEFD]);
			}*/
		
			return true;
		}
    }
	
}

inline bool Shell::isExecutableFileExist (const std::string& name) {
	if(name.find_first_of(".") != string::npos) return false;
	else{
		struct stat buffer;   
		string path = "/net/gcs/103/0356038/ras/";
		path = path + name;
		return (stat (path.c_str(), &buffer) == 0); 
	}
}

int Shell::isSupportedExternalCommand(string s){
	int loc;
	if((loc = s.find("printenv")) != string::npos){
		return 0;
	}else if((loc = s.find("setenv")) != string::npos){
		return 1;
	}else if((loc = s.find("batch")) != string::npos){
		return 2;
	}else return -1;
}

bool Shell::executeExternalCommand(int select, string s){
	if(select == 0) return printENV(s);
	else if(select == 1) return setENV(s);
	else if(select == 2) return runBatch(s);
	else{
		cerr << "Invalid External Command" << endl;
		return false;
	}
}

bool Shell::printENV(string s){
	int loc = s.find("printenv");
	s = s.substr(loc + 8);
	int loc1 = s.find_first_not_of(" ");
	if(loc1 == string::npos){
		cerr << "Invalid use of [printenv [environment variable]]" << endl;
		return false;
	}else s = s.substr(loc1,s.find_first_of(" \n", loc1) - loc1);
	
	//cout << s << endl;
	
	char* pPath;
	pPath = getenv (s.c_str());
	if (pPath != NULL) cout << s << "=" << pPath << endl;
	return true;
}

bool Shell::setENV(string s){
	int loc = s.find("setenv");
	s = s.substr(loc + 6);
	int loc1 = s.find_first_not_of(" ",loc);
	int loc2 = s.find_first_of(" ",loc1);
	if(loc1 == string::npos){
		cerr << "Invalid use of [setenv [environment variable] [new value]]" << endl;
		return false;
	}else;
	string environName = s.substr(loc1, loc2 - loc1);
	
	int loc3 = s.find_first_not_of(" ",loc2);
	int loc4 = s.find_first_of(" \n",loc3);
	if(loc3 == string::npos){
		cerr << "Invalid use of [setenv [environment variable] [new value]]" << endl;
		return false;
	}else;
	string replaceName = s.substr(loc3,loc4 - loc3);
	
	/*string currentPath = "/net/gcs/103/0356038/NP_1/";
	string newPath = currentPath + replaceName + "/";
	currentPath = newPath;*/
	//cerr << environName << endl;
	//cerr << replaceName << endl;
	
	setenv(environName.c_str(), replaceName.c_str(), 2);
   
	return true;
}

bool Shell::runBatch(string s){
	int loc = s.find("batch");
	s = s.substr(loc + 5);
	int loc1 = s.find_first_not_of(" ");
	if(loc1 == string::npos){
		cerr << "Invalid use of [batch [file name]]" << endl;
		return false;
	}else s = s.substr(loc1,s.find_first_of(" \n", loc1) - loc1);
	
	//cout << s << endl;
	
	
	std::ifstream infile(s.c_str());
	std::string line;
	while (std::getline(infile, line))
	{
		line += "\n";
		processCommand(line);
	}
	
	return true;
}

bool Shell::isExit(){
	return isExitStatus;
}

void Shell::reset(){
	m_PipePool.clear();
	isExitStatus = false;
}


