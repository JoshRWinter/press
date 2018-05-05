#include "press.h"

int main()
{
	press::write("\"the year is {}\"\n", 2018);
	press::write("\"the year is {5}\"\n", 2018);
	press::write("\"the year is {05}\"\n", 2018);
	press::write("\"the year is {-5}\"\n", 2018);
	press::write("\"pi is {.4}\"\n", 3.1415926);
	press::write("\"my name is {}\"\n", "Bob");
	press::writeln("\"this one ({}) has a newline at the end\"", "the coolest one");

	return 0;
}
