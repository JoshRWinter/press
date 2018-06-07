#include <string>
#include <stdlib.h>
#include <time.h>

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

int main()
{
	// integers and width/padding
	prwriteln("the year is {}", 2018);
	prwriteln("the year is { }", 2018);
	prwriteln("the year is {5}", 2018);
	prwriteln("the year is {05}", 2018);
	prwriteln("the year is {,}", 2018);
	prwriteln("the year is \"{-5}\"", 2018);

	// runtime width
	prwriteln("the year is {0}", press::set_width(2018, 10));

	// float
	prwriteln("pi is {.4}", 3.1415926);

	// float with runtime precision
	prwriteln("pi is {}", press::set_prec(3.1415926, 2));

	// hexadecimal and octal
	prwriteln("hex number: {x}", 859654u);
	prwriteln("hex number with 0-pad and width: {0x3}", 10u);
	prwriteln("uppercase hex number: 0x{X}", 859654u);
	prwriteln("octal number {o}", 1455587u);

	// string literal and std::string
	prwriteln("my name is {}", "Bob");
	prwriteln("std::string: {}", std::string("hello"));

	// limit printed characters
	prwriteln("my name is {.3}", "sam sampson");

	// booleans
	prwriteln("boolean: {}", false);

	// characters
	prwriteln("this char: {}", 'M');

	// void pointer
	prwriteln("void pointer 0x{}", main);

	// write to a buffer (like snprintf)
	char cool[13];
	prbwrite(cool, sizeof(cool), "cool {}", 12.23589f);
	prwriteln("string: {}, {} chars", cool, strlen(cool));

	// positional specifiers
	prwriteln("{@2}, {05@1}, {-4@2}", 31, 55);

	// write to a struct FILE*
	FILE *file = fopen("/tmp/test.txt", "w");
	if(file)
	{
		prfwriteln(file, "this is a cool {}", "function");
		fclose(file);
	}

	// user defined type
	my_custom_class mcc;
	prwriteln("custom type: {}", mcc);

	return 0;
}
