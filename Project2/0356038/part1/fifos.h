#ifndef __FIFOS_H__
#define __FIFOS_H__

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
using namespace std;

#define MAX_USER_NUM 30
#define MAX_BUF 1024

class FIFOS{
public:
	FIFOS();
	~FIFOS();
	
	// return -1 if the pipe does not exist
	// return fifo fd
	int getFifoToRead(int src, int dest);
	
	// return -1 if the pipe already used
	// return fifo fd
	int getFifoToWrite(int src, int dest);
	
	int getFifoToReadNoBlock(int src, int dest);
	int getFifoToWriteNoBlock(int src, int dest);
	string getFifoName(int src, int dest);
	void clearConnectedInfo(int id);
	
private:
	void findPipeNumByFd(int fd, int& src, int& dest);
	void clearFifoData(int fd);
	
	
	string m_fifoName[MAX_USER_NUM][MAX_USER_NUM];
	int m_fd[MAX_USER_NUM][MAX_USER_NUM];
	int m_writtenStatus[MAX_USER_NUM][MAX_USER_NUM];
};

#endif