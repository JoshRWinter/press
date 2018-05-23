#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <unordered_map>
#include <chrono>
#include <stdio.h>

std::unordered_map<const char*, std::chrono::time_point<std::chrono::high_resolution_clock>> starts;
std::unordered_map<const char*, unsigned long long> times;

#define BENCH_START(x) \
	if(times.find(#x) == times.end()) \
	{ \
		times[#x] = 0ull; \
	} \
	starts.insert({#x, std::chrono::high_resolution_clock::now()});

#define BENCH_STOP(x) \
	{ \
		const std::chrono::time_point<std::chrono::high_resolution_clock> x##end_time = std::chrono::high_resolution_clock::now(); \
		times[#x] += (unsigned long long)std::chrono::duration<float, std::micro>(x##end_time - starts[#x]).count(); \
	}

#define BENCH_SHOW(x) \
	printf("benchmark \"" #x "\" took %llu microseconds\n", times[#x]);

struct data
{
	const char *name;
	int age;
	unsigned children;
	unsigned long long houses;
	long long sandwiches;
};

data get_data();

#endif // BENCHMARK_H
