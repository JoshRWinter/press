#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>

#include "../press.h"
#include "benchmark.h"

int main()
{
	constexpr int loops = 1000000;

	for(int i = 0; i < loops; ++i)
	{
		const data d = get_data();

		BENCH_START("Press");
		char buffer[2048];
		prbwrite(buffer, sizeof(buffer), "Hello, my name is {}, I am {} years old, I have {} children, {} houses, and {} sandwiches.\n", d.name, d.age, d.children, d.houses, d.sandwiches);
		BENCH_STOP("Press");

		process(buffer);
	}

	for(int i = 0; i < loops; ++i)
	{
		const data d = get_data();

		BENCH_START("Printf");
		char buffer[2048];
		snprintf(buffer, sizeof(buffer), "Hello, my name is %s, I am %d years old, I have %u children, %llu houses, and %lld sandwiches.\n", d.name, d.age, d.children, d.houses, d.sandwiches);
		BENCH_STOP("Printf");

		process(buffer);
	}

	for(int i = 0; i < loops; ++i)
	{
		const data d = get_data();

		BENCH_START("Cout");
		std::ostringstream buffer;
		buffer << "Hello, my name is " << d.name << ", I am " << d.age << " years old, I have " << d.children << " children, " << d.houses << " houses, and " << d.sandwiches << " sandwiches." << std::endl;
		BENCH_STOP("Cout");

		process(buffer);
	}

	printf("------------------------------------------\n");
	BENCH_SHOW("Press");
	BENCH_SHOW("Printf");
	BENCH_SHOW("Cout");
	printf("------------------------------------------\n");

	return 0;
}
