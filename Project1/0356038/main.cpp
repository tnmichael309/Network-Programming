
#include "server.h"
#include <cstdlib>
#include <unistd.h>

int main(int arc, char* argv[]){

	if(chdir("./ras") != 0){
		cerr << "Change working directory failed" << endl;
		exit(0);
	}
	
	server s(atoi(argv[1]));
	
	return 0;
}
