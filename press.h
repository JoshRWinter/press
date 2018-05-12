#ifndef PRESS_H
#define PRESS_H

#include <type_traits>
#include <memory>
#include <cmath>

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

#include <tuple>
#define variadic_size(...) std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value

#define pressfmt(fmt, count) \
	static_assert(press::printer::is_balanced(fmt, press::specifier_list::string_length(fmt)), "press: specifier brackets are not balanced!"); \
	static_assert(press::printer::count_specifiers(fmt, press::printer::string_length(fmt)) >= count, "press: too many parameters!"); \
	static_assert(press::printer::count_specifiers(fmt, press::printer::string_length(fmt)) <= count, "press: not enough parameters!");

#define prwrite(fmt, ...) \
	pressfmt(fmt, variadic_size(__VA_ARGS__)) \
	press::write_unchecked(fmt, __VA_ARGS__)

#define prwriteln(fmt, ...) \
	pressfmt(fmt, variadic_size(__VA_ARGS__)) \
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

	static void abort(const std::string &msg)
	{
#ifdef PRESS_NO_EXCEPT
		fprintf(stderr, "press: %s\n", msg.c_str());
		std::abort();
#else
		throw std::runtime_error("press: " + msg);
#endif // PRESS_NO_EXCEPT
	}

	enum class print_target
	{
		STDOUT,
		STDERR,
		BUFFER
	};

	struct settings
	{
		settings(const char *fmt, unsigned first, unsigned len)
		{
			enum class states
			{
				READING_FLAGS,
				READING_WIDTH,
				READING_PRECISION,
				READING_INDEX
			};

			reset();
			states state = states::READING_FLAGS;

			for(unsigned i = 0; i < len;)
			{
				const char c = fmt[first + i];

				if(state == states::READING_FLAGS)
				{
					state = states::READING_WIDTH;

					switch(c)
					{
						case '0':
							zero_pad = true;
							break;
						case '-':
							left_justify = true;
							break;
						default:
							continue;
					}
				}
				else if(state == states::READING_WIDTH)
				{
					if(c == '.')
					{
						state = states::READING_PRECISION;
						continue;
					}
					else if(c == '@')
					{
						state = states::READING_INDEX;
						continue;
					}
					else if(!isdigit(c))
					{
						reset();
						break;
					}

					width *= 10;
					width += c - '0';
				}
				else if(state == states::READING_PRECISION)
				{
					if(c != '.')
					{
						if(c == '@')
						{
							state = states::READING_INDEX;
							continue;
						}
						if(!isdigit(c))
						{
							reset();
							break;
						}

						precision *= 10;
						precision += c - '0';
					}
				}
				else if(state == states::READING_INDEX)
				{
					if(c != '@')
					{
						if(!isdigit(c))
						{
							reset();
							break;
						}

						if(index == -1)
							index = 0;

						index *= 10;
						index += c - '0';
					}
				}

				++i;
			}

			if(width > 20)
				width = 20;
			if(precision > 10)
				precision = 10;
		}

		void reset(){ zero_pad = false; left_justify = false; width = 0; precision = 0; index = -1; }

		// flags
		bool zero_pad;
		bool left_justify;

		unsigned width;
		unsigned precision;
		int index; // starts at 1
	};

	class writer
	{
	public:
		static const int DEFAULT_BUFFER_SIZE = 1024;

		writer(print_target target, char *const user_buffer = NULL, const unsigned user_buffer_size = 0)
			: m_target(target)
			, m_buffer(target == print_target::BUFFER ? user_buffer : m_automatic_buffer)
			, m_bookmark(0)
			, m_size(target == print_target::BUFFER ? user_buffer_size : DEFAULT_BUFFER_SIZE)
		{}
		writer(const writer&) = delete;
		writer(writer&&) = delete;
		void operator=(const writer&) = delete;
		void operator=(writer&&) = delete;

		inline void write(const char *const buf, const unsigned count)
		{
			const unsigned written = std::min(m_size - m_bookmark, count);

			memcpy(m_buffer + m_bookmark, buf, written);
			m_bookmark += written;

			if(written < count && flush())
				write(buf + written, count - written);
		}

		bool flush()
		{
			switch(m_target)
			{
				case print_target::STDOUT:
					fwrite(m_buffer, 1, m_bookmark, stdout);
					break;
				case print_target::STDERR:
					fwrite(m_buffer, 1, m_bookmark, stderr);
					break;
				case print_target::BUFFER:
					// can't flush
					return false;
			}

			printf("|");
			m_bookmark = 0;
			return true;
		}

		inline int size() const { return m_size; }
		inline char *buffer() const { return m_buffer; }

	private:
		const print_target m_target;
		char *const m_buffer;
		unsigned m_bookmark; // first unwritten byte
		const unsigned m_size;
		char m_automatic_buffer[DEFAULT_BUFFER_SIZE];
	};

	class parameter
	{
	public:
		enum class ptype : unsigned char
		{
			FLOAT64,
			FLOAT32,
			SIGNED_INT,
			UNSIGNED_INT,
			BOOLEAN_,
			CHARACTER,
			STRING,
			POINTER
		};

		parameter() : type(ptype::FLOAT64), object(NULL) {}

		void init(const double &d)
		{
			type = ptype::FLOAT64;
			object = &d;
		}

		void init(const float &f)
		{
			type = ptype::FLOAT32;
			object = &f;
		}

		void init(const signed long long &i)
		{
			type = ptype::SIGNED_INT;
			object = &i;
		}

		void init(const unsigned long long &i)
		{
			type = ptype::UNSIGNED_INT;
			object = &i;
		}

		void init(const bool &b)
		{
			type = ptype::BOOLEAN_;
			object = &b;
		}

		void init(const char &c)
		{
			type = ptype::CHARACTER;
			object = &c;
		}

		void init(const char *s)
		{
			type = ptype::STRING;
			object = s;
		}

		void init(const void *v)
		{
			type = ptype::POINTER;
			object = v;
		}

		void convert(writer &buffer, const settings &format) const
		{
			switch(type)
			{
				case ptype::SIGNED_INT:
					convert_int(buffer, format);
					break;
				case ptype::UNSIGNED_INT:
					convert_uint(buffer, format);
					break;
				case ptype::STRING:
					convert_string(buffer, format);
				case ptype::FLOAT32:
					convert_float32(buffer, format);
					break;
				case ptype::CHARACTER:
					convert_uint(buffer, format);
					break;
				case ptype::FLOAT64:
					convert_float64(buffer, format);
					break;
				case ptype::BOOLEAN_:
					convert_bool(buffer, format);
					break;
				case ptype::POINTER:
					convert_pointer(buffer, format);
					break;
				default:
					abort("unrecognized enum case: " + std::to_string((int)type));
					break;
			}
		}

	private:
		static void reverse(char *buff, unsigned len)
		{
			unsigned index = 0;
			unsigned opposite = len - 1;
			while(index < opposite)
			{
				char *a = buff + index;
				char *b = buff + opposite;

				const char tmp = *a;
				*a = *b;
				*b = tmp;

				++index;
				--opposite;
			}
		}

		template <typename T> static inline unsigned int_length(T i)
		{
			if(!std::is_integral<T>::value)
				abort("must be an integer here");

			int negative = 0;
			if(std::is_signed<T>::value)
			{
				negative = i < 0;
				i = std::llabs(i);
			}

			unsigned len = 0;
			if(i < 10)
				len = 1;
			else if(i < 100)
				len = 2;
			else if(i < 1000)
				len = 3;
			else if(i < 10000)
				len = 4;
			else
			{
				while(i)
				{
					i /= 10;
					++len;
				}
			}

			return len + negative;
		}

		template <typename T> static unsigned stringify_int(char *buffer, T i)
		{
			if(!std::is_integral<T>::value)
				abort("must be an integer here");

			bool negative = false;
			if(std::is_signed<T>::value)
			{
				negative = i < 0;
				i = std::llabs(i);
			}

			unsigned place = 0;
			while(i)
			{
				buffer[place++] = (i % 10) + '0';
				i /= 10;
			}
			if(negative)
				buffer[place++] = '-';

			reverse(buffer, place);

			return place;
		}

		void convert_float64(writer &buffer, const settings &format) const
		{
		}

		void convert_float32(writer &buffer, const settings &format) const
		{
		}

		void convert_uint(writer &buffer, const settings &format) const
		{
		}

		void convert_int(writer &buffer, const settings &format) const
		{
			char string[20];
			const unsigned written = stringify_int(string, *(long long*)object);

			unsigned max = std::max(written, format.width);
			const char pad = format.zero_pad ? '0' : ' ';
			for(unsigned i = max; i > written; --i)
				buffer.write(&pad, 1);

			buffer.write(string, written);
		}

		void convert_string(writer &buffer, const settings &format) const
		{
		}

		void convert_bool(writer &buffer, const settings &format) const
		{
		}

		void convert_character(writer &buffer, const settings &format) const
		{
		}

		void convert_pointer(writer &buffer, const settings &format) const
		{
		}

		ptype type;
		const void *object;
	};

	constexpr unsigned string_length(const char *fmt, unsigned count = 0, unsigned index = 0)
	{
		return
		(fmt[index] == 0) ?
			(count)
			: (string_length(fmt, count + 1, index + 1));
	}

	constexpr bool is_literal_brace(const char *fmt, unsigned len, unsigned index)
	{
		return
		(index > len - 3) ?
			(false)
			: (fmt[index] == '{' && fmt[index + 1] == '{' && fmt[index + 2] == '}');
	}

	constexpr bool is_balanced(const char *fmt, unsigned len, unsigned index = 0, bool open = false)
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

	constexpr unsigned find_partner(const char *fmt, unsigned len, unsigned index)
	{
		return
		(fmt[index] == '}') ?
			(index)
			: (find_partner(fmt, len, index + 1));
	}

	constexpr unsigned count_specifiers(const char *fmt, unsigned len, unsigned count = 0, unsigned index = 0)
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

	void print_plain(const char *fmt, unsigned start, unsigned spec_begin, writer &buffer)
	{
		const char *const substr = fmt + start;
		const char *const position = strstr(substr, "{{}");
		const bool found = position != NULL && position < fmt + spec_begin;
		const unsigned index = found == false ? -1 : (position - fmt);
		const unsigned len = found == false ? spec_begin - start : (index - start);

		if(found == false)
			buffer.write(substr, len);
		else
		{
			buffer.write(substr, len + 1);
			print_plain(fmt, index + 3, spec_begin, buffer);
		}
	}

	void printer(const char *const fmt, const parameter *const params, const unsigned param_count, bool checked, const print_target target)
	{
		const unsigned fmt_len = strlen(fmt);

		// consistency checks
		if(checked)
		{
			if(!is_balanced(fmt, fmt_len))
				abort("specifier brackets are not balanced");

			const unsigned spec_count = count_specifiers(fmt, fmt_len);
			if(spec_count < param_count)
				abort("too many parameters!");
			else if(spec_count > param_count)
				abort("not enough parameters");
		}

		// buffering
		writer output(target);

		// begin printing
		unsigned bookmark = 0;
		for(unsigned k = 0; k < param_count; ++k)
		{
			// find the first open specifier bracket and extract the spec
			unsigned spec_len = 0;
			unsigned spec_begin = bookmark;
			unsigned spec_end = spec_begin;
			for(; spec_begin < fmt_len; ++spec_begin)
			{
				const char c = fmt[spec_begin];

				if(c == '{')
				{
					if(is_literal_brace(fmt, fmt_len, spec_begin))
					{
						spec_begin += 2;
						continue;
					}
					else
					{
						spec_end = find_partner(fmt, fmt_len, spec_begin);
						spec_len = spec_end - spec_begin - 1;
						break;
					}
				}
			}

			// print the "before text"
			print_plain(fmt, bookmark, spec_begin, output);

			settings format_settings(fmt, spec_begin + 1, spec_len);
			params[k].convert(output, format_settings);

			bookmark = spec_end + 1;
		}

		if(bookmark < fmt_len)
			print_plain(fmt, bookmark, fmt_len, output);

		output.flush();
	}

	template <typename T> inline void add(const T&, parameter*, unsigned&) { printf("unknown"); }

	// meaningfull specializations
	inline void add(const unsigned long long &x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const long long &x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const char &x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const double &x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const float &x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const char* x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const bool &x, parameter *array, unsigned &index) { array[index++].init(x); }

	// specializations that delegate to other specializations
	inline void add(const unsigned long &x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	inline void add(const unsigned &x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	inline void add(const unsigned short &x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	inline void add(const unsigned char &x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	inline void add(const signed long &x, parameter *array, unsigned &index) { add((long long)x, array, index); }
	inline void add(const int &x, parameter *array, unsigned &index) { add((long long)x, array, index); }
	inline void add(const short &x, parameter *array, unsigned &index) { add((long long)x, array, index); }

	// interfaces

	const int DEFAULT_AUTO_SIZE = 10;
	template <typename... Ts> void write(const char *fmt, const Ts&... ts)
	{
		parameter *storage;
		std::unique_ptr<parameter[]> dynamic;
		parameter automatic[DEFAULT_AUTO_SIZE];
		if(sizeof...(Ts) > DEFAULT_AUTO_SIZE)
		{
			dynamic.reset(new parameter[sizeof...(Ts)]);
			storage = dynamic.get();
		}
		else
		{
			storage = automatic;
		}

		#if defined (__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
		#endif

		#if defined (__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
		#endif

		unsigned index = 0;
		const char dummy[sizeof...(Ts)] = { (add(ts, storage, index), (char)1)... };

		#if defined (__GNUC__)
		#pragma GCC diagnostic pop
		#endif

		#if defined (__GNUC__)
		#pragma GCC diagnostic pop
		#endif

		printer(fmt, storage, sizeof...(Ts), true, print_target::STDOUT);
	}
}

#endif // PRESS_H
