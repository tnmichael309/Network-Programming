#include <cstring>
#include <iostream>

#include "pipePool.h"

using namespace std;

class Shell{
public:
	
	void processCommand(string s);
	bool executeCommand(string s);
	bool createProcess(string s, Pipe readPipeToDestroy, int fd[2]);
	
	int isSupportedExternalCommand(string s); // return -1 if invalid
	bool executeExternalCommand(int select, string s);
	bool printENV(string s); // external command 0
	bool setENV(string s); // external command 1
	bool runBatch(string s);
	bool isExit();
private:
	PipePool m_PipePool;
	string currentPath;
	inline bool isExecutableFileExist(const string& name);
	bool isExitStatus;
};