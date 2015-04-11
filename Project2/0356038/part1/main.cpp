
#include "server.h"
#include <cstdlib>
#include <unistd.h>

#include "client.h"

int main(int arc, char* argv[]){

	if(chdir("../ras") != 0){
		cerr << "Change working directory failed" << endl;
		exit(0);
	}
	
	
	server serv;
	serv.start(atoi(argv[1]));
	
	/*clientPools cp;
	int id = cp.addUser("140.113.210.145", "2088",13);
	int id2 = cp.addUser("140.113.210.146", "2088",14);
	cp.setName(id2, "Michael");
	cp.showAllUsers(id);
	cp.showAllUsers(id2);
	
	cp.delUser(0);
	cp.showAllUsers(id);
	
	id = cp.addUser("140.113.210.147", "2088",15);
	cp.setName(id, "Michael");
	cp.showAllUsers(id);
	cp.showAllUsers(id2);
	
	cp.showAllUsers(cp.findUserByFD(15));*/
	
	return 0;
}
