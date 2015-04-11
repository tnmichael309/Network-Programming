#include "proxyServer.h"

#include <iostream>
using namespace std;

int main(int argc, char** argv){
	
	server proxyServer;
	proxyServer.start(atoi(argv[1]), atoi(argv[2]));
	
	return 0;
}