#ifndef PRESS_H
#define PRESS_H

#include <cassert>
#include <string>
#include <type_traits>
#include <memory>

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
			: m_fmt(format.c_str())
			, m_len(format.length())
			, m_bookmark(0)
			, m_spec_number(0)
			, m_specs(new std::pair<unsigned, unsigned>[arg_count])
		{
			// consistency checks
			const bool balanced = is_balanced();
			const int spec_count = count_specifiers();

			if(!balanced)
				abort("specifer brackets are not balanced!");
			else if(arg_count < spec_count)
				abort("not enough parameters to print function!");
			else if(arg_count > spec_count)
				abort("too many parameters to print function!");

			// store the specifiers
			find_specs();
		}

		template <typename T> void output(const T &arg)
		{
			const char *const format_string = get_format_string(arg);
			const unsigned spec_begin = m_specs[m_spec_number].first;
			const unsigned spec_len = m_specs[m_spec_number].second;
			++m_spec_number;

			print_plain(m_bookmark, spec_begin);
			m_bookmark = spec_begin + spec_len;

			char format[20];
			if(std::is_pointer<typename std::remove_reference<T>::type>::value)
			{
				printf("%p", (void*)&arg);
				return;
			}
			else if(format_string[0] == 's')
			{
				const std::string argument = to_string(arg);
				snprintf(format, sizeof(format), "%%%.*ss", spec_len - 2, m_fmt + spec_begin + 1);
				printf(format, argument.c_str());
				return;
			}
			else
			{
				snprintf(format, sizeof(format), "%%%.*s%s", spec_len - 2, m_fmt + spec_begin + 1, format_string);
				printf(format, arg);
				return;
			}
		}

		void print_plain(unsigned start, unsigned spec_begin)
		{
			const char *const substr = m_fmt + start;
			const char *const found = strstr(substr, "{{}");
			const unsigned index = found == NULL ? -1 : (found - m_fmt);
			const unsigned len = found == NULL ? spec_begin - start : (index - start);

			if(found == NULL || index >= spec_begin)
				printf("%.*s", len, substr);
			else
			{
				printf("%.*s", len + 1, substr);
				print_plain(index + 3, spec_begin);
			}
		}

		unsigned bookmark() const
		{
			return m_bookmark;
		}

	private:
		const char *const m_fmt;
		const unsigned m_len;
		unsigned m_bookmark;
		int m_spec_number;
		std::unique_ptr<std::pair<unsigned, unsigned>[]> m_specs; // <beginning, length>

		void find_specs()
		{
			int index = 0;

			for(unsigned i = 0; i < m_len; ++i)
			{
				const char c = m_fmt[i];

				if(c == '{')
				{
					if(is_literal_brace(i))
					{
						i += 2;
					}
					else
					{
						m_specs[index].first = i;
						m_specs[index].second = find_partner(i) - i + 1;
						++index;
					}
				}
			}
		}

		bool is_literal_brace(unsigned index)
		{
			return index <= m_len - 3 && m_fmt[index] == '{' && m_fmt[index + 1] == '{' && m_fmt[index + 2] == '}';
		}

		bool is_balanced()
		{
			bool open = false;

			for(unsigned i = 0; i < m_len; ++i)
			{
				const char c = m_fmt[i];

				if(c == '{')
				{
					if(open)
						return false;

					open = true;

					if(is_literal_brace(i))
						++i;
				}
				else if(c == '}' && open)
				{
					open = false;
				}
			}

			return !open;
		}

		int count_specifiers()
		{
			int count = 0;

			for(unsigned i = 0; i < m_len; ++i)
			{
				const char c = m_fmt[i];

				if(c == '{')
				{
					if(is_literal_brace(i))
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

		unsigned find_partner(unsigned index)
		{
			for(unsigned i = index; i < m_len; ++i)
			{
				const char c = m_fmt[i];

				if(c == '}')
					return i;
			}

			abort("could not find closing brace");
			return 0;
		}

		static void abort(const std::string &msg)
		{
			fprintf(stderr, "press: %s\n", msg.c_str());
			std::abort();
		}
	};

	// interfaces

	template <typename... Ts> void write(const std::string &fmt, const Ts&... ts)
	{

#if defined (__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
		printer p(fmt, sizeof...(Ts));

		const char dummy[sizeof...(Ts)] = { (p.output(ts), (char)1)... };

#if defined (__GNUC__)
#pragma GCC diagnostic pop
#endif

		const unsigned bookmark = p.bookmark();
		if(bookmark < fmt.length())
			p.print_plain(bookmark, fmt.length());
	}

	template <typename... Ts> void writeln(const std::string &fmt, const Ts&... ts)
	{
		write(fmt, ts...);
		puts("");
	}
}

#endif // PRESS_H
