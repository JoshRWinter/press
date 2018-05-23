#include "benchmark.h"

#define BENCH_PRESS
// #define BENCH_PRINTF
// #define BENCH_COUT

#ifdef BENCH_PRESS
#include "../press.h"
void process(const char*);
#endif
#ifdef BENCH_PRINTF
#include <stdio.h>
void process(const char*);
#endif
#ifdef BENCH_COUT
#include <iostream>
#include <sstream>
#include <string>
void process(const std::ostringstream&);
#endif

int main()
{
	BENCH_START(perf);

	for(int i = 0; i < 1000000; ++i)
	{
		const data d = get_data();

#ifdef BENCH_PRESS
		char buffer[2048];
		prbwrite(buffer, sizeof(buffer), "Hello, my name is {}, I am {} years old, I have {} children, {} houses, and {} sandwiches.\n", d.name, d.age, d.children, d.houses, d.sandwiches);
#endif
#ifdef BENCH_PRINTF
		char buffer[2048];
		snprintf(buffer, sizeof(buffer), "Hello, my name is %s, I am %d years old, I have %u children, %llu houses, and %lld sandwiches.\n", d.name, d.age, d.children, d.houses, d.sandwiches);
#endif
#ifdef BENCH_COUT
		std::ostringstream buffer;
		buffer << "Hello, my name is " << d.name << ", I am " << d.age << " years old, I have " << d.children << " children, " << d.houses << " houses, and " << d.sandwiches << " sandwiches." << std::endl;
#endif

		process(buffer);
	}

	BENCH_STOP(perf);
printf("------------------------------------------\n");
	BENCH_SHOW(perf);
printf("------------------------------------------\n");
}
