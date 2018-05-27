#ifndef PRESS_H
#define PRESS_H

#include <type_traits>
#include <memory>
#include <algorithm>
#include <string>

#include <string.h>
#include <ctype.h>
#include <limits.h>
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

#include <tuple>
#define variadic_size(...) std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value

#define pressfmtcheck(fmt, count) \
	static_assert(press::is_balanced(fmt, strlen(fmt)), "press: specifier brackets are not balanced!"); \
	static_assert(press::count_specifiers(fmt, strlen(fmt)) >= count, "press: too many parameters!");

#define prwrite(fmt, ...) \
	pressfmtcheck(fmt, variadic_size(__VA_ARGS__)) \
	press::write_(press::print_target::FILE_P, stdout, NULL, 0u, fmt, ##__VA_ARGS__)

#define prfwrite(fp, fmt, ...) \
	pressfmtcheck(fmt, variadic_size(__VA_ARGS__)) \
	press::write_(press::print_target::FILE_P, fp, NULL, 0u, fmt, ##__VA_ARGS__)

#define prbwrite(userbuffer, size, fmt, ...) \
	pressfmtcheck(fmt, variadic_size(__VA_ARGS__)) \
	press::write_(press::print_target::BUFFER, NULL, userbuffer, size, fmt, ##__VA_ARGS__)

namespace press
{
	enum class print_target
	{
		FILE_P,
		BUFFER
	};

	struct settings
	{
		settings()
		{
			reset();
		}

		static int parse(const char *const fmt, const unsigned first, const unsigned len, settings &s)
		{
			unsigned bookmark = first;

			// consume flags
			if(bookmark >= len)
				return bookmark;
			if(fmt[bookmark] == '0')
			{
				s.zero_pad = true;
				++bookmark;
			}
			else if(fmt[bookmark] == '-')
			{
				s.left_justify = true;
				++bookmark;
			}

			// consume width
			if(bookmark >= len)
				return bookmark;
			s.width = consume_number(fmt, bookmark, len);

			// consume precision
			if(bookmark >= len)
				return bookmark;
			if(fmt[bookmark] == '.')
			{
				++bookmark;
				s.precision = consume_number(fmt, bookmark, len);
			}

			// consume index
			if(bookmark >= len)
				return bookmark;
			if(fmt[bookmark] == '@')
			{
				++bookmark;
				s.index = consume_number(fmt, bookmark, len);
			}

			return bookmark;
		}

		void reset(){ zero_pad = false; left_justify = false; width = -1; precision = -1; index = -1; }

	private:
		static signed char consume_number(const char *const fmt, unsigned &bookmark, const unsigned len)
		{
			unsigned char number = 0;
			bool found = false;

			while(bookmark < len && isdigit(fmt[bookmark]))
			{
				found = true;
				number *= 10;
				number += fmt[bookmark++] - '0';
			}

			return found ? number : -1;
		}
	public:

		// flags
		bool zero_pad;
		bool left_justify;

		signed char width;
		signed char precision;
		signed char index; // starts at 1
	};

	class writer
	{
	public:
		writer(print_target target, FILE *fp, char *const user_buffer, const unsigned user_buffer_size)
			: m_target(target)
			, m_fp(fp)
			, m_buffer(user_buffer)
			, m_bookmark(0)
			, m_size(user_buffer_size)
		{}
		writer(const writer&) = delete;
		writer(writer&&) = delete;
		~writer()
		{
			if(m_target == print_target::BUFFER)
				m_buffer[m_bookmark >= m_size ? m_size - 1 : m_bookmark] = 0;
		}

		inline void write(const char *const buf, const unsigned count)
		{
			switch(m_target)
			{
				case print_target::FILE_P:
					fwrite(buf, 1, count, m_fp);
					break;
				case print_target::BUFFER:
				{
					const unsigned written = std::min(m_size - m_bookmark, count);

					memcpy(m_buffer + m_bookmark, buf, written);
					m_bookmark += written;
					break;
				}
			}
		}

	private:
		const print_target m_target;
		FILE *const m_fp;
		char *const m_buffer;
		unsigned m_bookmark; // first unwritten byte
		const unsigned m_size;
	};

	class hex;
	class HEX;
	class oct;
	class ptr;
	class parameter
	{
		friend hex;
		friend HEX;
		friend oct;
		friend ptr;

	public:
		enum class ptype : unsigned char
		{
			NONE,
			FLOAT64,
			SIGNED_INT,
			UNSIGNED_INT,
			BOOLEAN_,
			CHARACTER,
			BUFFER,
			CUSTOM
		};

		parameter() : type (ptype::NONE) {}

		~parameter()
		{
			if(type == ptype::CUSTOM)
			{
				typedef std::string sstring;
				sstring *s = (sstring*)object.rawbuf;
				s->~sstring();
			}
		}

		void init(const double d)
		{
			type = ptype::FLOAT64;
			object.f64 = d;
		}

		void init(const signed long long i)
		{
			type = ptype::SIGNED_INT;
			object.lli = i;
		}

		void init(const unsigned long long i)
		{
			type = ptype::UNSIGNED_INT;
			object.ulli = i;
		}

		void init(const bool b)
		{
			type = ptype::BOOLEAN_;
			object.b = b;
		}

		void init(const char c)
		{
			type = ptype::CHARACTER;
			object.c = c;
		}

		void init(const char *s)
		{
			type = ptype::BUFFER;
			object.cstr = s;
		}

		void init(std::string &&str)
		{
			type = ptype::CUSTOM;
			new (object.rawbuf) std::string(std::move(str));
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
				case ptype::BUFFER:
					convert_string(buffer, format);
					break;
				case ptype::CHARACTER:
					convert_character(buffer, format);
					break;
				case ptype::FLOAT64:
					convert_float64(buffer, format);
					break;
				case ptype::BOOLEAN_:
					convert_bool(buffer, format);
					break;
				case ptype::CUSTOM:
					convert_custom(buffer, format);
					break;
				default:
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

		template <typename T> static unsigned stringify_int(char *buffer, T i)
		{
			if(!std::is_integral<T>::value)
				return 0;

			bool negative = false;
			if(std::is_signed<T>::value)
			{
				if(i == LLONG_MIN)
				{
					memcpy(buffer, "-9223372036854775808", 20);
					return 20;
				}
				negative = i < 0;
				i = std::llabs(i);
			}

			unsigned place = 0;
			if(i == 0)
				buffer[place++] = '0';
			else
			{
				while(i)
				{
					buffer[place++] = (i % 10) + '0';
					i /= 10;
				}
			}
			if(negative)
				buffer[place++] = '-';

			reverse(buffer, place);

			return place;
		}

		static unsigned stringify_int_hex(char *buffer, unsigned long long i, bool uppercase)
		{
			unsigned place = 0;
			const char base_character = uppercase ? 'A' : 'a';

			if(i == 0)
				buffer[place++] = '0';
			else
			{
				while(i)
				{
					const char h = (i % 16);
					if(h < 10)
						buffer[place++] = h + '0';
					else
						buffer[place++] = (h - 10) + base_character;
					i /= 16;
				}
			}

			reverse(buffer, place);
			return place;
		}

		static unsigned stringify_int_oct(char *buffer, unsigned long long i)
		{
			unsigned place = 0;
			if(i == 0)
				buffer[place++] = '0';
			else
			{
				while(i)
				{
					buffer[place++] = (i % 8) + 48;
					i /= 8;
				}
			}

			reverse(buffer, place);
			return place;
		}

		void convert_float64(writer &buffer, const settings &format) const
		{
			char buf[325];
			const int written = snprintf(buf, sizeof(buf), "%.*f", format.precision >= 0 ? format.precision : 6, object.f64);
			const int min = std::min(324, written);
			buffer.write(buf, min);
		}

		void convert_uint(writer &buffer, const settings &format) const
		{
			char string[20];
			const unsigned written = stringify_int(string, object.ulli);

			unsigned width = format.width >= 0 ? format.width : 0;
			unsigned max = std::max(written, width);
			const char pad = format.zero_pad ? '0' : ' ';

			if(!format.left_justify)
				for(unsigned i = max; i > written; --i)
					buffer.write(&pad, 1);

			buffer.write(string, written);

			const char space = ' ';
			if(format.left_justify)
				for(unsigned i = max; i > written; --i)
					buffer.write(&space, 1);
		}

		void convert_int(writer &buffer, const settings &format) const
		{
			char string[20];
			const unsigned written = stringify_int(string, object.lli);

			unsigned width = format.width >= 0 ? format.width : 0;
			unsigned max = std::max(written, width);
			const char pad = format.zero_pad ? '0' : ' ';

			if(!format.left_justify)
				for(unsigned i = max; i > written; --i)
					buffer.write(&pad, 1);

			buffer.write(string, written);

			if(format.left_justify)
				for(unsigned i = max; i > written; --i)
					buffer.write(&pad, 1);
		}

		void convert_string(writer &buffer, const settings &format) const
		{
			const auto len = strlen(object.cstr);
			buffer.write(object.cstr, len);
		}

		void convert_bool(writer &buffer, const settings &format) const
		{
			buffer.write(object.b ? "true" : "false", object.b ? 4 : 5);
		}

		void convert_character(writer &buffer, const settings &format) const
		{
			char c = object.c;
			buffer.write(&c, 1);
		}

		void convert_custom(writer &buffer, const settings &format) const
		{
			const std::string *s = (const std::string*)object.rawbuf;
			buffer.write(s->c_str(), s->length());
		}

		ptype type;
		union
		{
			long long lli;
			unsigned long long ulli;
			double f64;
			char c;
			bool b;
			const char *cstr;
			char rawbuf[sizeof(std::string)];
		}object;
	};

	struct hex
	{
		hex(unsigned long long i)
		{
			const unsigned written = parameter::stringify_int_hex(m_buffer, i, false);
			m_buffer[written] = 0;
		}

		char m_buffer[17];
	};

	struct HEX
	{
		HEX(unsigned long long i)
		{
			const unsigned written = parameter::stringify_int_hex(m_buffer, i, true);
			m_buffer[written] = 0;
		}

		char m_buffer[17];
	};

	struct oct
	{
		oct(unsigned long long i)
		{
			const unsigned written = parameter::stringify_int_oct(m_buffer, i);
			m_buffer[written] = 0;
		}

		char m_buffer[23];
	};

	struct ptr
	{
		ptr(const void *p)
		{
			unsigned long long number = reinterpret_cast<uintptr_t>(p);
			const unsigned written = parameter::stringify_int_hex(m_buffer, number, false);
			m_buffer[written] = 0;
		}

		char m_buffer[17];
	};

	constexpr bool is_literal_brace(const char *fmt, unsigned len, unsigned index)
	{
		return
		(index > len - 3) ?
			(false)
			: (fmt[index] == '{' && fmt[index + 1] == '{' && fmt[index + 2] == '}');
	}

	constexpr bool is_balanced(const char *fmt, unsigned len, unsigned index = 0, int open = 0)
	{
		return
			(index >= len) ?
				(open == 0)
				: ((fmt[index] == '{') ?
					(is_literal_brace(fmt, len, index) ?
						(is_balanced(fmt, len, index + 3, open))
						: (is_balanced(fmt, len, index + 1, open + 1)))
					: ((fmt[index] == '}') ?
						((open > 0) ?
							(is_balanced(fmt, len, index + 1, open - 1))
							: (is_balanced(fmt, len, index + 1, open)))
						: (is_balanced(fmt, len, index + 1, open))));
	}

	constexpr int find_partner(const char *fmt, unsigned len, unsigned index)
	{
		return
		(index >= len) ?
			(-1)
			: ((fmt[index] == '}') ?
				(index)
				: (find_partner(fmt, len, index + 1)));
	}

	constexpr unsigned count_specifiers(const char *fmt, unsigned len, unsigned count = 0, unsigned index = 0)
	{
		return
		(index >= len) ?
			(count)
			: ((fmt[index] != '{') ?
				(count_specifiers(fmt, len, count, index + 1))
				: ((is_literal_brace(fmt, len, index)) ?
					(count_specifiers(fmt, len, count, index + 3))
					: ((find_partner(fmt, len, index + 1) == -1) ?
						(count)
						: (count_specifiers(fmt, len, count + 1, find_partner(fmt, len, index + 1) + 1)))));
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

	void printer(const char *const fmt, const parameter *const params, const unsigned pack_size, const print_target target, FILE *fp, char *userbuffer, const unsigned userbuffer_size)
	{
		const unsigned fmt_len = strlen(fmt);
		const unsigned spec_count = count_specifiers(fmt, fmt_len);

		// buffering
		writer output(target, fp, userbuffer, userbuffer_size);

		// begin printing
		unsigned bookmark = 0;
		for(unsigned k = 0; k < spec_count; ++k)
		{
			// find the first open specifier bracket and extract the spec
			unsigned spec_begin = bookmark;
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
						break;
					}
				}
			}

			// print the "before text"
			print_plain(fmt, bookmark, spec_begin, output);

			settings format_settings;
			bookmark = settings::parse(fmt, spec_begin + 1, fmt_len, format_settings) + 1;

			const bool spec_index_overridden = format_settings.index >= 0;
			const int index = spec_index_overridden ? format_settings.index - 1 : k;
			if(spec_index_overridden && (index < 0 || index >= (int)pack_size))
				output.write("{UNDEFINED}", 11);
			else if(!spec_index_overridden && index >= (int)pack_size)
				output.write("{UNDEFINED}", 11);
			else
				params[index].convert(output, format_settings);
		}

		if(bookmark < fmt_len)
			print_plain(fmt, bookmark, fmt_len, output);
	}

	template <typename T> std::string to_string(const T&)
	{
		return "{UNKNOWN DATA TYPE}";
	}

	template <typename T> inline void add(const T &x, parameter *array, unsigned &index)
	{
		array[index++].init(std::move(press::to_string(x)));;
	}

	// meaningfull specializations
	inline void add(const unsigned long long x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const long long x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const char x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const double x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const char *x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const bool x, parameter *array, unsigned &index) { array[index++].init(x); }
	inline void add(const std::string &x, parameter *array, unsigned &index) { array[index++].init(x.c_str()); }
	inline void add(const hex &x, parameter *array, unsigned &index) { array[index++].init(x.m_buffer); }
	inline void add(const HEX &x, parameter *array, unsigned &index) { array[index++].init(x.m_buffer); }
	inline void add(const oct &x, parameter *array, unsigned &index) { array[index++].init(x.m_buffer); }
	inline void add(const ptr &x, parameter *array, unsigned &index) { array[index++].init(x.m_buffer); }

	// specializations that delegate to other specializations
	inline void add(const unsigned long x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	inline void add(const unsigned x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	inline void add(const unsigned short x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	inline void add(const unsigned char x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	inline void add(const signed long x, parameter *array, unsigned &index) { add((long long)x, array, index); }
	inline void add(const int x, parameter *array, unsigned &index) { add((long long)x, array, index); }
	inline void add(const short x, parameter *array, unsigned &index) { add((long long)x, array, index); }
	inline void add(const float x, parameter *array, unsigned &index) { array[index++].init((double)x); }

	// interfaces

	const int DEFAULT_AUTO_SIZE = 10;
	template <typename... Ts> inline void write_(print_target target, FILE *fp, char *userbuffer, unsigned userbuffer_size, const char *fmt, const Ts&... ts)
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

		printer(fmt, storage, sizeof...(Ts), target, fp, userbuffer, userbuffer_size);
	}

	template <typename... Ts> void write(const char *fmt, const Ts&... ts)
	{
		write_(print_target::FILE_P, stdout, NULL, 0, fmt, ts...);
	}

	template <typename... Ts> void fwrite(FILE *fp, const char *fmt, const Ts&... ts)
	{
		write_(print_target::FILE_P, fp, NULL, 0, fmt, ts...);
	}

	template <typename... Ts> void bwrite(char *userbuffer, unsigned userbuffer_size, const char *fmt, const Ts&... ts)
	{
		write_(print_target::BUFFER, NULL, userbuffer, userbuffer_size, fmt, ts...);
	}
}

#endif // PRESS_H
