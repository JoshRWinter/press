#include "benchmark.h"
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

data get_data()
{
	return {"joe biden", 47, 33, 78, -111222558};
}

void process(const char *str)
{
	const char *expected = "Hello, my name is joe biden, I am 47 years old, I have 33 children, 78 houses, and -111222558 sandwiches.";
	if(strcmp(expected, str))
	{
		fprintf(stderr, "%s", ("error: expected:\n\"" + std::string(expected) + "\"\ngot:\n\"" + std::string(str) + "\"").c_str());
		abort();
	}
}

void process(const std::ostringstream &ss)
{
	process(ss.str().c_str());
}
