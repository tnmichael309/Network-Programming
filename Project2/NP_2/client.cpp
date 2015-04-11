#include "client.h"

singleClient::singleClient(){
	m_name = "no name";
	m_isActive = true;
	m_isNewUser = true;
	m_shell = new Shell;
	m_shell->reset();
	
	// clear all environment variable
	extern char **environ;
	environ = new char*[2];
	environ[0] = new char[sizeof("PATH=bin:.")+1];
	strcpy(environ[0], "PATH=bin:.");
	environ[1] = NULL;
	
	m_environ.push_back("PATH=bin:.");
};

singleClient::~singleClient(){;}

void singleClient::init(string name, string ip, string port, int sockfd){
	m_name = name;
	m_ip = ip;
	m_port = port;
	m_sockfd = sockfd;
	m_isActive = true;
	m_isNewUser = true;
	m_environ.clear();
	m_environ.push_back("PATH=bin:.");
}

void singleClient::setName(string name){
	m_name = name;
}

string singleClient::getName(){
	return m_name;
}

string singleClient::getIP(){
	return m_ip;
}

string singleClient::getPort(){
	return m_port;
}

Shell* singleClient::getShell(){
	return m_shell;
}

int singleClient::getSockfd(){
	return m_sockfd;
}

bool singleClient::isActiveUser(){
	return m_isActive;
}

bool singleClient::isNewUser(){
	// if query once, it's known and become old user
	bool result = m_isNewUser;
	m_isNewUser = false;
	return result;
}

void singleClient::setAvtiveStatus(bool status){
	m_isActive = status;
	if(status == false){
		m_shell->reset();
	}
}

void singleClient::setOldUser(){
	m_isNewUser = false;
}

void singleClient::setSelfEnvironment(){
	/*cout << "content: " << endl;
	for(int i =0; i < m_environ.size(); i++){
		cout << m_environ[i] << endl;
	}
	
	int count = 0;
	extern char **environ;
    printf("\n");
	cout << "environ content: " << endl;
    while(environ[count] != NULL)
	{
        printf("%s\n", environ[count]);
        count++;
	}*/
   
	extern char **environ;
	environ = new char*[m_environ.size() + 1];
	
	for(int i =0; i < m_environ.size(); i++){
		//cout << m_environ[i] << endl;
		environ[i] = new char[m_environ[i].size() + 1];
		strcpy(environ[i], m_environ[i].c_str());
	}
	environ[m_environ.size()] = NULL;
	
}

void singleClient::storeEnvironment(){
	m_environ.clear();
	
	int count = 0;
	extern char **environ;
    while(environ[count] != NULL)
	{
		m_environ.push_back(environ[count]);
        count++;
	}
}

int clientPools::addUser(string ip, string port, int sockfd){
	
	// whether there are empty slot to reuse
	for(int i = 0; i < m_vClients.size(); i++){
		if(m_vClients[i].isActiveUser() == false){
			m_vClients[i].init("no name", ip , port,sockfd);
			return i;
		}
	}
	
	// if no, create a new client and insert into the client pool
	singleClient newClient;
	newClient.init("no name", ip , port, sockfd);
	m_vClients.push_back(newClient);
	
	return m_vClients.size() - 1;
}

void clientPools::delUser(int id){
	// check if it is an valid id
	if(id >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	/*Shell* childShell = m_vClients[id].getShell();
	childShell->reset();*/
	m_vClients[id].setAvtiveStatus(false);
}

bool clientPools::setName(int id, string name){
	// whether there are empty slot to reuse
	for(int i = 0; i < m_vClients.size(); i++){
		if(m_vClients[i].isActiveUser() == true && m_vClients[i].getName() == name){
			return false;
		}
	}
	
	// check if it is an valid id
	if(id >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	
	m_vClients[id].setName(name);
}

int clientPools::findUserByFD(int fd){
	for(int i = 0; i < m_vClients.size(); i++){
		if(m_vClients[i].isActiveUser() == true && m_vClients[i].getSockfd() == fd){
			return i;
		}
	}
	return -1;
}

int clientPools::findUserFD(int curID){
	// check if it is an valid id
	if(curID >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	
	if(m_vClients[curID].isActiveUser() == true){
		return m_vClients[curID].getSockfd();
	}
	
	return -1;
}

int clientPools::getContainerSize(){
	return m_vClients.size();
}

bool clientPools::isActiveUser(int id){
	// check if it is an valid id
	if(id >= m_vClients.size()){
		return false;
	}
	
	return m_vClients[id].isActiveUser();
}

bool clientPools::isNewUser(int curID){
	// check if it is an valid id
	if(curID >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	
	if(m_vClients[curID].isActiveUser() == false){
		cerr << "Invalid ID in isOldUser function" << endl;
		while(1);
	}
	
	return m_vClients[curID].isNewUser();
}

string clientPools::getUserName(int curID){
	// check if it is an valid id
	if(curID >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	
	if(m_vClients[curID].isActiveUser() == false){
		cerr << "Invalid ID in isOldUser function" << endl;
		while(1);
	}
	
	return m_vClients[curID].getName();
}

string clientPools::getUserIP(int curID){
	// check if it is an valid id
	if(curID >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	
	if(m_vClients[curID].isActiveUser() == false){
		cerr << "Invalid ID in isOldUser function" << endl;
		while(1);
	}
	
	return m_vClients[curID].getIP();
}
string clientPools::getUserPort(int curID){
	// check if it is an valid id
	if(curID >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	
	if(m_vClients[curID].isActiveUser() == false){
		cerr << "Invalid ID in isOldUser function" << endl;
		while(1);
	}
	
	return m_vClients[curID].getPort();
}
Shell* clientPools::getClientShell(int curID){
	// check if it is an valid id
	if(curID >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	
	if(m_vClients[curID].isActiveUser() == false){
		cerr << "Invalid ID in isOldUser function" << endl;
		while(1);
	}
	
	// set environment using its own setting
	m_vClients[curID].setSelfEnvironment();
	
	return m_vClients[curID].getShell();
}
void clientPools::restoreClientShell(int curID){
	// check if it is an valid id
	if(curID >= m_vClients.size()){
		cerr << "Invalid id" << endl;
		while(1);
	}
	
	if(m_vClients[curID].isActiveUser() == false){
		cerr << "Invalid ID in isOldUser function" << endl;
		while(1);
	}
	
	// set environment using its own setting
	m_vClients[curID].storeEnvironment();
}

void clientPools::showAllUsers(int curID){
	cout << "<ID>\t<nickname>\t<IP/port>\t<indicate me>" << endl;
	
	for(int i = 0; i < m_vClients.size(); i++){
		if(m_vClients[i].isActiveUser() == true){
			cout << i+1 << "\t";
			if(m_vClients[i].getName() == "no name") cout << "(no name)";
			else cout << m_vClients[i].getName();

			cout << "\t" << m_vClients[i].getIP() << "/" << m_vClients[i].getPort();

			if(curID == i) cout << "\t<-me";
			cout << endl;
		}
	}
}