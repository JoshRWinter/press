#ifndef PRESS_H
#define PRESS_H

#include <type_traits>
#include <memory>

#include <string.h>
#include <stdio.h>

// #define PRESS_NO_EXCEPT
#ifndef PRESS_NO_EXCEPT
#include <stdexcept>
#endif // PRESS_NO_EXCEPT

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

#define pressfmt(fmt, count) \
	static_assert(press::specifier_list::is_balanced(fmt, press::specifier_list::string_length(fmt)), "press: specifier brackets are not balanced!"); \
	static_assert(press::specifier_list::count_specifiers(fmt, press::specifier_list::string_length(fmt)) >= count, "press: too many parameters!"); \
	static_assert(press::specifier_list::count_specifiers(fmt, press::specifier_list::string_length(fmt)) <= count, "press: not enough parameters!");

#define prwrite(fmt, ...) \
	pressfmt(fmt, press::variadic_size(__VA_ARGS__)) \
	press::write_unchecked(fmt, __VA_ARGS__)

#define prwriteln(fmt, ...) \
	pressfmt(fmt, press::variadic_size(__VA_ARGS__)) \
	press::writeln_unchecked(fmt, __VA_ARGS__)

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

	template <typename... Ts> constexpr unsigned variadic_size(const Ts&... ts)
	{
		return sizeof...(Ts);
	}

	static void abort(const std::string &msg)
	{
#ifdef PRESS_NO_EXCEPT
		fprintf(stderr, "press: %s\n", msg.c_str());
		std::abort();
#else
		throw std::runtime_error("press: " + msg);
#endif // PRESS_NO_EXCEPT
	}

	class format_specifier
	{
	public:
		unsigned spec() const { return m_spec_index; }
		unsigned spec_len() const { return m_spec_len; }
		unsigned content() const { return m_content_index; }
		unsigned content_len() const { return m_content_len; }

		void spec(unsigned i) { m_spec_index = i; }
		void spec_len(unsigned l) { m_spec_len = l; }
		void content(unsigned i) { m_content_index = i; }
		void content_len(unsigned l) { m_content_len = l; }

	private:
		unsigned m_spec_index;
		unsigned m_spec_len;
		unsigned m_content_index;
		unsigned m_content_len;
	};

	class specifier_list
	{
	public:
		specifier_list(const char *format, unsigned pack_size, bool checked)
		{
			const unsigned len = strlen(format);

			// consistency checks
			if(checked)
			{
				const bool balanced = is_balanced(format, len);
				if(!balanced)
					abort("specifer brackets are not balanced!");

				const unsigned spec_count = count_specifiers(format, len);
				if(pack_size < spec_count)
					abort("not enough parameters to print function!");
				else if(pack_size > spec_count)
					abort("too many parameters to print function!");
			}

			// initialize storage
			if(pack_size > DEFAULT_ARRAY_SIZE)
			{
				m_dynamic_array.reset(new format_specifier[pack_size]);
				m_storage = m_dynamic_array.get();
			}
			else
			{
				m_storage = m_automatic_array;
			}

			// store the specifiers
			find_specs(format, len);
		}

		static constexpr unsigned string_length(const char *fmt, unsigned count = 0, unsigned index = 0)
		{
			return
			(fmt[index] == 0) ?
				(count)
				: (string_length(fmt, count + 1, index + 1));
		}

		static constexpr bool is_balanced(const char *fmt, unsigned len, unsigned index = 0, bool open = false)
		{
			return
			(index == len) ?
				(!open)
				: ((fmt[index] == '{') ?
					((is_literal_brace(fmt, len, index)) ?
						(is_balanced(fmt, len, index + 3, open))
						: (is_balanced(fmt, len, index + 1, true)))
					: ((fmt[index] == '}' && open) ?
						(is_balanced(fmt, len, index + 1, false))
						: (is_balanced(fmt, len, index + 1, open))));
		}

		static constexpr unsigned count_specifiers(const char *fmt, unsigned len, unsigned count = 0, unsigned index = 0)
		{
			return
			(index == len) ?
				(count)
				: ((fmt[index] == '{') ?
					((is_literal_brace(fmt, len, index)) ?
						(count_specifiers(fmt, len, count, index + 3))
						: (count_specifiers(fmt, len, count + 1, find_partner(fmt, len, index) + 1)))
					: (count_specifiers(fmt, len, count, index + 1)));
		}

		static constexpr bool is_literal_brace(const char *fmt, unsigned len, unsigned index)
		{
			return
			(index > len - 3) ?
				(false)
				: (fmt[index] == '{' && fmt[index + 1] == '{' && fmt[index + 2] == '}');
		}

		static constexpr unsigned find_partner(const char *fmt, unsigned len, unsigned index)
		{
			return
			(fmt[index] == '}') ?
				(index)
				: (find_partner(fmt, len, index + 1));
		}

		const format_specifier &operator[](unsigned i) const
		{
			return m_storage[i];
		}

	private:
		static const int DEFAULT_ARRAY_SIZE = 10;

		void find_specs(const char *fmt, unsigned len)
		{
			int index = 0;

			for(unsigned i = 0; i < len; ++i)
			{
				const char c = fmt[i];

				if(c == '{')
				{
					if(is_literal_brace(fmt, len, i))
					{
						i += 2;
					}
					else
					{
						m_storage[index].spec(i);
						m_storage[index].spec_len(find_partner(fmt, len, i) - i + 1);
						m_storage[index].content(i + 1);
						m_storage[index].content_len(m_storage[index].spec_len() - 2);

						++index;
					}
				}
			}
		}

		format_specifier *m_storage;
		format_specifier m_automatic_array[10];
		std::unique_ptr<format_specifier> m_dynamic_array;
	};

	class printer
	{
	public:
		printer(const char *format, unsigned arg_count, bool checked = true)
			: m_fmt(format)
			, m_len(strlen(format))
			, m_pack_size(arg_count)
			, m_bookmark(0)
			, m_spec_number(0)
			, m_specs(format, arg_count, checked)
		{}

		template <typename T> void output(const T &arg)
		{
			const char *const format_string = get_format_string(arg);
			const unsigned spec_begin = m_specs[m_spec_number].spec();
			const unsigned spec_len = m_specs[m_spec_number].spec_len();
			const unsigned content_begin = m_specs[m_spec_number].content();
			const unsigned content_len = m_specs[m_spec_number].content_len();
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
				snprintf(format, sizeof(format), "%%%.*ss", content_len, m_fmt + content_begin);
				printf(format, argument.c_str());
				return;
			}
			else
			{
				snprintf(format, sizeof(format), "%%%.*s%s", content_len, m_fmt + content_begin, format_string);
				printf(format, arg);
				return;
			}
		}

		void finish()
		{
			if(m_pack_size == m_spec_number && m_bookmark < m_len)
				print_plain(m_bookmark, m_len);
		}

	private:
		void print_plain(unsigned start, unsigned spec_begin)
		{
			const char *const substr = m_fmt + start;
			const char *const position = strstr(substr, "{{}");
			const bool found = position != NULL && position < m_fmt + spec_begin;
			const unsigned index = found == false ? -1 : (position - m_fmt);
			const unsigned len = found == false ? spec_begin - start : (index - start);

			if(found == false)
				printf("%.*s", len, substr);
			else
			{
				printf("%.*s", len + 1, substr);
				print_plain(index + 3, spec_begin);
			}
		}

		const char *const m_fmt;
		const unsigned m_len;
		const unsigned m_pack_size;
		unsigned m_bookmark;
		unsigned m_spec_number;
		const specifier_list m_specs;
	};

	// interfaces

	template <typename... Ts> void write(const char *fmt, const Ts&... ts)
	{
		#if defined (__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
		#endif

		printer p(fmt, sizeof...(Ts));
		const char dummy[sizeof...(Ts)] = { (p.output(ts), (char)1)... };
		p.finish();

		#if defined (__GNUC__)
		#pragma GCC diagnostic pop
		#endif
	}

	template <typename... Ts> void writeln(const char *fmt, const Ts&... ts)
	{
		#if defined (__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
		#endif

		printer p(fmt, sizeof...(Ts));
		const char dummy[sizeof...(Ts)] = { (p.output(ts), (char)1)... };
		p.finish();

		#if defined (__GNUC__)
		#pragma GCC diagnostic pop
		#endif

		puts("");
	}

	template <typename... Ts> void write_unchecked(const char *fmt, const Ts&... ts)
	{
		#if defined (__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
		#endif

		printer p(fmt, sizeof...(Ts), false);
		const char dummy[sizeof...(Ts)] = { (p.output(ts), (char)1)... };
		p.finish();

		#if defined (__GNUC__)
		#pragma GCC diagnostic pop
		#endif
	}

	template <typename... Ts> void writeln_unchecked(const char *fmt, const Ts&... ts)
	{
		#if defined (__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
		#endif

		printer p(fmt, sizeof...(Ts), false);
		const char dummy[sizeof...(Ts)] = { (p.output(ts), (char)1)... };
		p.finish();

		#if defined (__GNUC__)
		#pragma GCC diagnostic pop
		#endif

		puts("");
	}
}

#endif // PRESS_H
