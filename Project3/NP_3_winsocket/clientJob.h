#ifndef _CLIENT_JOB_H
#define _CLIENT_JOB_H

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
		m_lineNum = 0;
		//m_batchFile.open(file.c_str(), ios::in);

	}
	string getResultString(){
		return m_resultString;
	}
	void addToResultString(string addedString){
		while(addedString[addedString.size()-1] == 13) addedString.pop_back();

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

		fstream m_batchFile;
		m_batchFile.open(m_sSeverFile.c_str(), ios::in);
		int openResult = m_batchFile.is_open();
		int counter = m_lineNum;

		while(counter > 0){
			getline(m_batchFile, command);
		};

		// do command in batch file
		if(getline(m_batchFile, command)){
			m_lineNum ++;
			// wrap command append to m_resultString
			string wrappedCommand = "% ";
			wrappedCommand += "<b>";
			wrappedCommand += command;
			wrappedCommand += "</b>";
			addToResultString(wrappedCommand);

			// send command to server
			//cerr << m_jobID << " " << command << " " << command.size() << endl;
			command += "\n";
			send(m_serverSocket, command.c_str(), command.size(),0);

			if(command == "exit\n")isFinish = true;
			else isFinish = false;
		}else{
			isFinish = true;
		}
	}
	void clientJob::readUntilEnd(){
		char* buff;
		buff = new char[1024];
		memset(buff, 0, 1024*sizeof(char));

		while(recv(m_serverSocket, buff, 1024, 0) > 0){
			string rString(buff);
			//cerr << "receive string: " << rString << endl;

			if(rString.find('%') != string::npos) isReadyToWrite = true;
			
			stringstream ss(rString);
			string subString;
			while(getline(ss,subString,'\n')){ // divide string by '\n'
				int pos = 0;
				if((pos = subString.find('%')) != string::npos){
					subString = subString.substr(0,pos) + subString.substr(pos + 2);
				}
				if(subString != ""){
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
	int m_lineNum;
	bool isFinish;
	bool isReadyToWrite;
};
#endif