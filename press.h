#ifndef PRESS_H
#define PRESS_H

#include <cassert>
#include <string>
#include <type_traits>

#include <string.h>
#include <stdio.h>

/* PRESS printing tool

Press is a I/O tool for human-readable output using printf style syntax, but with extra type
safety and convenience.

- Format specifiers are denoted by {}, with optional arguments inside the braces
- Arguments inside the braces are simply forwarded to printf, specifically the sections:
	- Flag characters
	- Field width
	- Precision
  There is no need for the "Length modifier" or "Conversion specifier" sections, as these
  are deduced automatically
- Passing std::string& or char* is supported for strings
- If you need to write a literal "{}" in the string, only the first '{' needs to be escaped
- To escape a literal '{', enclose it in {} e.g. press::write("{{}") prints "{"

examples
1. press::write("the year is {}", 2018); // "the year is 2018"
2. press::write("the year is {5}", 2018); // "the year is  2018"
3. press::write("the year is {05}", 2018); // "the year is 02018"
4. press::write("the year is {-5}", 2018); // "the year is 2018 "
5. press::write("pi is {.4}", 3.1415926); // "pi is 3.1415"
6. press::write("my name is {}", "Bob"); // "my name is Bob" (the string argument can be std::string or char*)

*/

namespace press
{
	template <typename T> const char *get_format_string(const T&) { return "s"; }

	template <> const char *get_format_string(const unsigned long long int&) { return "llu"; }
	template <> const char *get_format_string(const unsigned long int&) { return "lu"; }
	template <> const char *get_format_string(const unsigned int&) { return "u"; }
	template <> const char *get_format_string(const unsigned short&) { return "hu"; }
	template <> const char *get_format_string(const unsigned char&) { return "hhu"; }

	template <> const char *get_format_string(const long long int&) { return "lld"; }
	template <> const char *get_format_string(const long int&) { return "ld"; }
	template <> const char *get_format_string(const int&) { return "d"; }
	template <> const char *get_format_string(const short&) { return "hd"; }
	template <> const char *get_format_string(const char&) { return "c"; }

	template <> const char *get_format_string(const long double&) { return "Lf"; }
	template <> const char *get_format_string(const double&) { return "f"; }
	template <> const char *get_format_string(const float&) { return "f"; }

	typedef void* void_pointer;
	template <> const char *get_format_string(const void_pointer&) { return "p"; }

	template <typename T> std::string to_string(const T&)
	{
		return "unrecognized type";
	}

	template <> std::string to_string(const std::string &str)
	{
		return str;
	}

	template <unsigned N> std::string to_string(const char (&str)[N])
	{
		return str;
	}

	void abort(const std::string &msg)
	{
		fprintf(stderr, "press: %s\n", msg.c_str());
		std::abort();
	}

	bool is_literal_brace(const std::string &fmt, unsigned index)
	{
		return fmt.find("{{}", index) == index;
	}

	bool is_balanced(const std::string &fmt)
	{
		const int len = fmt.length();
		bool open = false;

		for(int i = 0; i < len; ++i)
		{
			const char c = fmt[i];

			if(c == '{')
			{
				if(open)
					return false;

				open = true;

				if(is_literal_brace(fmt, i))
					++i;
			}
			else if(c == '}' && open)
			{
				open = false;
			}
		}

		return !open;
	}

	int count_specifiers(const std::string &fmt)
	{
		const int len = fmt.length();
		int count = 0;

		for(int i = 0; i < len; ++i)
		{
			const char c = fmt[i];

			if(c == '{')
			{
				if(is_literal_brace(fmt, i))
				{
					++i;
					continue;
				}
				else
				{
					++count;
				}
			}
		}

		return count;
	}

	int find_partner(const std::string &fmt, int index)
	{
		const int len = fmt.length();

		for(int i = index; i < len; ++i)
		{
			const char c = fmt[i];

			if(c == '}')
				return i;
		}

		abort("could not find closing brace");
		return 0;
	}

	template <typename T> void output(const std::string &fmt, int &index, const T &arg)
	{
		const int start = index;

		// find the specifier
		std::string spec;
		const int len = fmt.length();
		for(int i = index; i < len; ++i)
		{
			const char c = fmt[i];

			if(c == '{')
			{
				printf("%.*s", i - start, fmt.c_str() + start);
				const int end = find_partner(fmt, i);
				index = end + 1;
				spec = fmt.substr(i + 1, end - i - 1);

				if(spec == "{")
				{
					printf("{");
					output(fmt, index, arg);
					return;
				}

				break;
			}
		}

		char format[20];

		if(std::is_pointer<typename std::remove_reference<T>::type>::value)
		{
			printf("%p", (void*)&arg);
			return;
		}
		else if(get_format_string(arg)[0] == 's')
		{
			const std::string argument = to_string(arg);
			snprintf(format, sizeof(format), "%%%ss", spec.c_str());
			printf(format, argument.c_str());
			return;
		}
		else
		{
			snprintf(format, sizeof(format), "%%%s%s", spec.c_str(), get_format_string(arg));
			printf(format, arg);
			return;
		}
	}

	// interface
	template <typename... Ts> void write(const std::string &fmt, const Ts&... ts)
	{
		const bool balanced = is_balanced(fmt);
		const int arg_count = sizeof...(Ts);
		const int spec_count = count_specifiers(fmt);

		if(arg_count < spec_count)
			abort("not enough parameters to print function!");
		else if(arg_count > spec_count)
			abort("too many parameters to print function!");
		else if(!balanced)
			abort("specifer brackets are not balanced!");

#if defined (__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

		int index = 0;
		const char dummy[sizeof...(Ts)] = { (output(fmt, index, ts), 1)... };

#if defined (__GNUC__)
	#pragma GCC diagnostic pop
#endif

		if(index < (int)fmt.length())
			printf("%.*s", (int)fmt.length() - index, fmt.c_str() + index);
	}
}

#endif // PRESS_H
