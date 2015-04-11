all:
	g++ -std=c++11 -O3 main.cpp pipePool.cpp shell.cpp server.cpp -o linux_csh
bsd:
	g++48 -std=c++11 -O3 main.cpp pipePool.cpp shell.cpp server.cpp -o unix_csh

background:
	nohup ./csh 2048 &
