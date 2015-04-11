#include "clientJob.h"
#include <unistd.h>

static std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

clientJob::clientJob(){
	m_sSeverIP = "";
	m_sServerPort = "";
}

void clientJob::init(string ip, string port, string file, int jobID){
	m_sSeverIP = ip;
	m_sServerPort = port;
	m_sSeverFile = file;
	m_jobID = jobID;
	m_resultString = "";
	m_batchFile.open(m_sSeverFile.c_str(), ios::in);
}

clientJob::~clientJob(){;}

void clientJob::getServerInfo(string& ip, string& port){
	ip = m_sSeverIP;
	port = m_sServerPort;
}

bool clientJob::doAJob(){
	
	// do job according to each command
	// and wrap in m_resultString
	string command = "";

	// do command in batch file
	if(getline(m_batchFile, command)){
		
		for(int i = command.size() -1; i > 0; i--){
			if(command[i] == 13) command.pop_back();
		}
		
		// wrap command append to m_resultString
		string wrappedCommand = "% ";
		wrappedCommand += "<b>";
		wrappedCommand += command;
		wrappedCommand += "</b>";
		addToResultString(wrappedCommand);
		
		// send command to server
		//cerr << m_jobID << " " << command << " " << command.size() << endl;
		command += "\n";
		write(m_serverFD, command.c_str(), command.size());
			
		if(command.find("exit") != string::npos) return false;
		else return true;
	}else{
		return false;
	}
}

string clientJob::getResultString(){
	string lastString = m_resultString;
	m_resultString = "";
	return lastString;
}

void clientJob::setConnectedSocketFD(int fd){
	m_serverFD = fd;
}

void clientJob::readUntilEnd(bool& isReadyToWrite){
	
	char* buff;
	buff = new char[4096];
	memset(buff, 0, 4096*sizeof(char));
	
	if(recv(m_serverFD, buff, 4096, 0) > 0){
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

void clientJob::addToResultString(string addedString){
	while(addedString != "" && addedString[addedString.size()-1] == 13) addedString.pop_back();
	
	m_resultString += "<script>document.all['m";
	m_resultString += to_string(m_jobID);
	m_resultString += "'].innerHTML += \"";
	m_resultString += addedString;
	m_resultString += "<br>\";</script>\n";	
}