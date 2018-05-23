all:
	g++ -o libdynamic.so -fpic -O2 -std=c++11 dynamic.cpp -shared -s
	g++ -o bench -O2 -std=c++11 benchmark.cpp -L. -ldynamic -s
	LD_LIBRARY_PATH=. ./bench
	ls -lh bench
