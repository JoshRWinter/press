#include <string>

struct my_custom_class
{
	my_custom_class() : t(time(NULL)) {}
	const time_t t;
};
namespace press
{
	std::string to_string(const my_custom_class &mcc)
	{
		return "the time is " + std::to_string(mcc.t);
	}
}

#include "press.h"

#include <time.h>

int main()
{
	prwrite("\"the year is {}\"\n", 2018);
	prwrite("\"the year is {5}\"\n", 2018u);
	prwrite("\"the year is {05}\"\n", 2018ul);
	prwrite("\"the year is {-5}\"\n", 2018ll);
	prwrite("\"pi is {.4}\"\n", 3.1415926);
	prwrite("\"hex number: {}\"\n", press::hex(859654));
	prwrite("\"uppercase hex number: 0x{}\"\n", press::HEX(859654));
	prwrite("\"octal number {}\"\n", press::oct(1455587));
	prwrite("\"my name is {}\"\n", "Bob");

	prwrite("boolean: {}, char: {}\n",true, '.');
	prwrite("void pointer 0x{}\n", press::ptr((void*)main));
	prwrite("std::string: {}\n", std::string("hello"));
	char cool[13];
	prbwrite(cool, sizeof(cool), "cool {}", 12.23589f);
	prwrite("string: {}\n", cool);

	prwrite("\"{@2}, {05@1}, {-4@2}\"\n", 31, 55);

	FILE *file = fopen("/tmp/test.txt", "w");
	if(file)
	{
		prfwrite(file, "this is a cool {}", "function");
		fclose(file);
	}

	// user defined type
	my_custom_class mcc;
	prwrite("\"custom type: {@1}\"\n", mcc);

	return 0;
}
