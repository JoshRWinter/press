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

static void tests();

int main()
{
	// run all the tests
	tests();

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

	// write to a std::string
	const std::string stdstr = prswriteln("{}, some more numbers {}", 3+9, 32);
	press::write("std::string: {}", stdstr);

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

static int number = 1;
template <typename... Ts> static void check(const char *expected, const char *fmt, const Ts&... ts)
{
	const std::string got = press::swrite(fmt, ts...);
	if(got != expected)
	{
		fprintf(stderr, "error!! expected \"%s\", got \"%s\"\n", expected, got.c_str());
		exit(1);
	}
	else
		fprintf(stderr, "check #%d passed (\"%s\")\n", number, got.c_str());

	++number;
}

void tests()
{
	fprintf(stderr, "============== running tests ==============\n");

	check("literal brace check: { {} coolio {}}}  {{ !", "literal brace check: {{} {{}} {} {{}}}}  {{}{{} !", "coolio");

	// integer padding
	check("integer: 42", "integer: {}", 42);
	check("blank padded integer:   43", "blank padded integer: {4}", 43);
	check("this (28    ) is a left justified padded integer", "this ({-6}) is a left justified padded integer", 28);
	check("this (000000899) is a zero-padded number", "this ({09}) is a zero-padded number", 899);
	check("this is a thousands separated number: 25,147,236", "this is a thousands separated number: {,}", 25147236);
	check("this is a blank-padded and thousands separated number:    2,225,225", "this is a blank-padded and thousands separated number: {,12}", 2225225);
	check("this is a left-justified and thousands separated number: 1,225,225,225       ", "this is a left-justified and thousands separated number: {,-20}", 1225225225);
	check("this signed integer has a blank space in front of it:  44 ", "this signed integer has a blank space in front of it: { -4}", 44);

	// alternate bases
	check("this right here (c) is a hexa-decimal number", "this right here ({x}) is a hexa-decimal number", 12u);
	check("this right here (12) is an octal number", "this right here ({o}) is an octal number", 10u);

	// strings
	check("this is a string: coolio julio", "this is a string: {}", "coolio julio");
	check("this is a std::string: coolio julio", "this is a std::string: {}", std::string("coolio julio"));

	fprintf(stderr, "============== all tests passed ==============\n");
}
