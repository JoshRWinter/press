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
5. press::write("pi is {.4}", 3.1415926); // "pi is 3.1416"
6. press::write("my name is {}", "Bob"); // "my name is Bob" (the string argument can be std::string or char*)

*/

namespace press
{
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

	class printer
	{
	public:
		printer(const std::string &format, int arg_count)
			: m_fmt(format)
			, m_spec_number(0)
			, m_bookmark(0)
		{
			const bool balanced = is_balanced(m_fmt);
			const int spec_count = count_specifiers(m_fmt);

			if(arg_count < spec_count)
				abort("not enough parameters to print function!");
			else if(arg_count > spec_count)
				abort("too many parameters to print function!");
			else if(!balanced)
				abort("specifer brackets are not balanced!");
		}

		template <typename T> void output(const T &arg)
		{
			int start;
			int length;
			find_spec(m_spec_number++, start, length);
			const std::string spec = m_fmt.substr(start + 1, length - 2);
			const char *const format_string = get_format_string(arg);

			char format[20];

			print_plain(m_bookmark, start - 1);
			m_bookmark = start + length;

			if(std::is_pointer<typename std::remove_reference<T>::type>::value)
			{
				printf("%p", (void*)&arg);
				return;
			}
			else if(format_string[0] == 's')
			{
				const std::string argument = to_string(arg);
				snprintf(format, sizeof(format), "%%%ss", spec.c_str());
				printf(format, argument.c_str());
				return;
			}
			else
			{
				snprintf(format, sizeof(format), "%%%s%s", spec.c_str(), format_string);
				printf(format, arg);
				return;
			}
		}

		void print_plain(unsigned first, unsigned last)
		{
			const auto pos = m_fmt.find("{{}", first);

			if(pos == std::string::npos || pos > last)
			{
				printf("%.*s", last - first + 1, m_fmt.c_str() + first);
			}
			else
			{
				print_plain(first, pos - 1);
				printf("{");
				print_plain(pos + 3, last);
			}
		}

		int bookmark() const
		{
			return m_bookmark;
		}

	private:
		const std::string &m_fmt;
		int m_spec_number;
		int m_bookmark;

		void find_spec(int number, int &start, int &length)
		{
			const int len = m_fmt.length();

			int i;
			for(i = 0; i < len; ++i)
			{
				const char c = m_fmt[i];

				if(c == '{')
				{
					if(is_literal_brace(m_fmt, i))
					{
						i += 2;
					}
					else if(number-- == 0)
					{
						const int partner = find_partner(m_fmt, i);
						start = i;
						length = partner - i + 1;
						return;
					}
				}
			}

			abort("couldn't find spec " + std::to_string(number));
		}

		static void abort(const std::string &msg)
		{
			fprintf(stderr, "press: %s\n", msg.c_str());
			std::abort();
		}

		static bool is_literal_brace(const std::string &fmt, unsigned index)
		{
			return fmt.find("{{}", index) == index;
		}

		static bool is_balanced(const std::string &fmt)
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

		static int count_specifiers(const std::string &fmt)
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

		static int find_partner(const std::string &fmt, int index)
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
	};

	// interface
	template <typename... Ts> void write(const std::string &fmt, const Ts&... ts)
	{

#if defined (__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
		printer p(fmt, sizeof...(Ts));

		int index = 0;
		const char dummy[sizeof...(Ts)] = { (p.output(ts), (char)1)... };

#if defined (__GNUC__)
#pragma GCC diagnostic pop
#endif

		const int bookmark = p.bookmark();
		if((unsigned)bookmark < fmt.length())
			p.print_plain(bookmark, fmt.length() - 1);
	}
}

#endif // PRESS_H
