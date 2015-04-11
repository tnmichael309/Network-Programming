#ifndef __PIPEPOOL_H__
#define __PIPEPOOL_H__

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdlib.h> 
using namespace std;

#define READFD 0
#define WRITEFD 1

class Pipe{
public:
	int writeFD; // fd to write to pipe
	int readFD; // fd to read from pipe
	int countDown; 
};

class PipePool{
public:
	PipePool();
	~PipePool();
	Pipe& getPipeToWrite(int countDown);
	bool getPipeToRead(Pipe& pipeToCopy);
	void updatePipeCountDown();
	void showPipeStatus();
	void updateLastState();
	void recover(int readPipeFd);
	void clear();
private:
	vector<Pipe> m_Pipes;
	vector<Pipe> m_lastPipes;
};

#endif