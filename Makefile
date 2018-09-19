all:
	g++ -o test -Wall -std=c++11 -g test.cpp

benchmark:
	make -C demos
