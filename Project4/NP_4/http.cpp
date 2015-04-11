#include "httpServer.h"

#include <iostream>
using namespace std;

int main(int argc, char** argv){
	
	server httpServer;
	httpServer.start(atoi(argv[1]));
	
	return 0;
}