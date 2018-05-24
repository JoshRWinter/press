#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <unordered_map>
#include <chrono>
#include <stdio.h>

std::unordered_map<const char*, std::chrono::time_point<std::chrono::high_resolution_clock>> starts;
std::unordered_map<const char*, unsigned long long> times;

#define BENCH_START(x) \
	if(times.find(x) == times.end()) \
	{ \
		times[x] = 0; \
	} \
	starts[x] = std::chrono::high_resolution_clock::now();

#define BENCH_STOP(x) \
	{ \
		const std::chrono::time_point<std::chrono::high_resolution_clock> benchmark_end_time_ = std::chrono::high_resolution_clock::now(); \
		times[x] += (unsigned long long)std::chrono::duration<double, std::nano>(benchmark_end_time_ - starts[x]).count(); \
	}

#define BENCH_SHOW(x) \
	printf("benchmark \"" x "\" took %llu microseconds\n", times[x] / 1000);

struct data
{
	const char *name;
	int age;
	unsigned children;
	unsigned long long houses;
	long long sandwiches;
};

data get_data();
void process(const char*);
void process(const std::ostringstream&);

#endif // BENCHMARK_H
