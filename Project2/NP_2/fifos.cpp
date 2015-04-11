#include "fifos.h"

FIFOS::FIFOS(){
	
	for(int i = 0; i < MAX_USER_NUM; i++){
		for(int j = 0; j < MAX_USER_NUM; j++){
			string fifoString = "../temp/";
			fifoString += to_string(i); // src
			fifoString += "to";
			fifoString += to_string(j); // dest
			fifoString += ".txt";
			
			m_fifoName[i][j] = fifoString;
			m_writtenStatus[i][j] = 0;
			m_fd[i][j] = -1;
		}
	}
}

FIFOS::~FIFOS(){;}

// return -1 if the pipe does not exist
// return fifo fd
int FIFOS::getFifoToRead(int src, int dest){
	if(m_writtenStatus[src][dest] == 0) return -1; // no one write the pipe before
	else{
		m_writtenStatus[src][dest] = 0;	
		return 1;
	}
}	

// return -1 if the pipe already used
// return fifo fd
int FIFOS::getFifoToWrite(int src, int dest){
	
	if(m_writtenStatus[src][dest] >= 1) return -1; // already written in this pipe
	else{
		m_writtenStatus[src][dest] += 1;
		return 1;
	}
}

int FIFOS::getFifoToReadNoBlock(int src, int dest){
	close(m_fd[src][dest]);
	m_fd[src][dest] = open(m_fifoName[src][dest].c_str(), O_RDONLY, S_IRUSR | S_IWUSR);
	cout << src << " " << dest << " " << m_fd[src][dest] << endl;
	return m_fd[src][dest];
}	

int FIFOS::getFifoToWriteNoBlock(int src, int dest){
	close(m_fd[src][dest]);
	m_fd[src][dest] = open(m_fifoName[src][dest].c_str(), O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	cout << src << " " << dest << " " << m_fd[src][dest] << endl;
	return m_fd[src][dest];
}

string FIFOS::getFifoName(int src, int dest){
	//cout << src << " " << dest << " " << m_fifoName[src][dest] << endl;
	return m_fifoName[src][dest];
}
	
void FIFOS::clearConnectedInfo(int id){
	for(int i = 0; i < MAX_USER_NUM; i++){
		for(int j = 0; j < MAX_USER_NUM; j++){
			m_writtenStatus[i][j] = 0;
			if(m_fd[i][j] != -1){
				//cout << "closed: " << m_fd[i][j] << endl;
				close(m_fd[i][j]);
			}
		}
	}
};

void FIFOS::findPipeNumByFd(int fd, int& src, int& dest){
	for(int i = 0; i < MAX_USER_NUM; i++){
		for(int j = 0; j < MAX_USER_NUM; j++){
			if(fd == m_fd[i][j]){
				src = i;
				dest = j;
				return;
			}
		}
	}
	
	cerr << "Invalid fd" << endl;
	return;
};

void FIFOS::clearFifoData(int fd){
	if(fd < 0) return;
	else{
		// clear data
		char buf[10000];
		read(fd, buf,10000);
	}
};


