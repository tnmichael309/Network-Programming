#include "pipePool.h"


PipePool::PipePool(){;}
PipePool::~PipePool(){;}

Pipe& PipePool::getPipeToWrite(int countDown){
	
	// find a pipe that pipe to the same process
	// should use the same pipe to write!
	for(int i = 0; i < m_Pipes.size(); i++){
		if(m_Pipes[i].countDown == countDown){
			//updatePipeCountDown();
			return m_Pipes[i];
		}
	}
	
	// empty or no such pipe
	// create a new pipe
	int pipefd[2];
	if (pipe(pipefd) < 0) {
		cerr << "pipe" << endl;
		exit(1);
	}
	
	Pipe newPipe;
	newPipe.countDown = countDown;
	newPipe.readFD = pipefd[READFD];
	newPipe.writeFD = pipefd[WRITEFD];
	m_Pipes.push_back(newPipe);
	
	//updatePipeCountDown();
	
	return m_Pipes[m_Pipes.size() - 1];
}

// is there any pipe to read from
// if true, copy the pipe data into "pipeToCopy"
// else return false and do nothing
bool PipePool::getPipeToRead(Pipe& pipeToCopy){
	for(int i = 0; i < m_Pipes.size(); i++){
		if(m_Pipes[i].countDown == 0){
			pipeToCopy = m_Pipes[i];
			m_Pipes.erase(m_Pipes.begin()+i);
			return true;
		}
	}
	return false;
}

void PipePool::updatePipeCountDown(){
	for(int i = 0; i < m_Pipes.size(); i++){
		m_Pipes[i].countDown --;
	}
}

void PipePool::showPipeStatus(){
	cout << "Pipe pool status:\n";
	for(int i = 0; i < m_Pipes.size(); i++){
		cout << "Pipe " << i << ": " << m_Pipes[i].readFD << " " << m_Pipes[i].writeFD << " " << m_Pipes[i].countDown << endl;
	}
}

void PipePool::updateLastState(){
	m_lastPipes.clear();
	for(int i = 0; i < m_Pipes.size(); i++){
		m_lastPipes.push_back(m_Pipes[i]);
	}
}

void PipePool::recover(int readPipeFd){
	if(readPipeFd == 0){
		// just recover countdown
		// no pipe was destroyed
		for(int i = 0; i < m_Pipes.size(); i++){
			m_Pipes[i].countDown ++;
		}
		return;
	}
	
	m_Pipes.clear();
	int indexToChangeSetting = 0;
	for(int i = 0; i < m_lastPipes.size(); i++){
		m_Pipes.push_back(m_lastPipes[i]); // recover last state of pipe to destroy
		if(m_lastPipes[i].readFD == readPipeFd){
			indexToChangeSetting = i;
		}
	}
	
	// create a new pipe
	int pipefd[2];
	if (pipe(pipefd) < 0) {
		cerr << "pipe" << endl;
		exit(1);
	}
	
	m_Pipes[indexToChangeSetting].readFD = pipefd[READFD];
	m_Pipes[indexToChangeSetting].writeFD = pipefd[WRITEFD];
	
	
	// read all data from readPipeFd 
	char dataBuffer[10000];
	int sizeOfData = read(readPipeFd, dataBuffer, sizeof(dataBuffer));
	
	if(sizeOfData <= 0) cerr << "Pipe reading error" << endl;

	// and write to the new pipe
	if(sizeOfData != write(pipefd[WRITEFD], dataBuffer, sizeOfData)) cerr << "Writing to new pipe error" << endl;
}

void PipePool::clear(){
	m_lastPipes.clear();
	for(int i = 0; i < m_Pipes.size(); i++){
		close(m_Pipes[i].writeFD);
		close(m_Pipes[i].readFD);
	}
	m_Pipes.clear();
}	
	