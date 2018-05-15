#include "press.h"

int main()
{
	press::write("\"the year is {}\"\n", 2018);
	press::write("\"the year is {5}\"\n", 2018u);
	press::write("\"the year is {05}\"\n", 2018ul);
	press::write("\"the year is {-5}\"\n", 2018ll);
	press::write("\"pi is {.4}\"\n", 3.1415926);
	press::write("\"my name is {}\"\n", "Bob");
	press::write("boolean: {}, char: {}\n",true, '.');
	press::write("void pointer {}\n", (void*)main);
	press::write("std::string: {}\n", std::string("hello"));
	char cool[13];
	press::bwrite(cool, sizeof(cool), "cool {}", 12.23589f);
	press::write("string: {}\n", cool);

	FILE *file = fopen("/tmp/test.txt", "w");
	press::fwrite(file, "this is a cool {}", "function");
	fclose(file);
	return 0;
}
