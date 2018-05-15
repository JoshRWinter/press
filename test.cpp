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

	return 0;
}
