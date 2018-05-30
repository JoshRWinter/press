#ifndef PRESS_H
#define PRESS_H

#include <type_traits>
#include <memory>
#include <algorithm>
#include <string>
#include <tuple>

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

#define pressfmtcheck(fmt, count) \
	static_assert(press::is_balanced(fmt, press::string_length(fmt)), "press: specifier brackets are not balanced!"); \
	static_assert(press::count_specifiers(fmt, press::string_length(fmt)) >= count, "press: too many parameters!")

#define prwrite(fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::write_(press::print_target::FILE_P, stdout, NULL, 0u, fmt, ##__VA_ARGS__)

#define prwriteln(fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::writeln(fmt, ##__VA_ARGS__)

#define prfwrite(fp, fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::write_(press::print_target::FILE_P, fp, NULL, 0u, fmt, ##__VA_ARGS__)

#define prfwriteln(fp, fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::fwriteln(fp, fmt, ##__VA_ARGS__)

#define prbwrite(userbuffer, size, fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::write_(press::print_target::BUFFER, NULL, userbuffer, size, fmt, ##__VA_ARGS__)

#define prbwriteln(userbuffer, size, fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::bwriteln(userbuffer, size, fmt, ##__VA_ARGS__)

namespace press
{
	constexpr int string_length(const char *s, int len = 0)
	{
		return s[len] == 0 ? len : string_length(s, len + 1);
	}

	template <typename T> struct width_spec
	{
		inline width_spec(const T &t, signed char v) : arg(t), value(v) {}
		const T &arg;
		const signed char value;
	};

	template <typename T> struct precision_spec
	{
		inline precision_spec(const T &t, signed char v) : arg(t), value(v) {}
		const T &arg;
		const signed char value;
	};

	template <typename T> struct width_precision_spec
	{
		inline width_precision_spec(const T &t, signed char v1, signed char v2) : arg(t), w(v1), p(v2) {}
		const T &arg;
		const signed char w;
		const signed char p;
	};

	template <typename T> inline width_spec<T> set_width(const T &arg, signed char w)
	{
		return width_spec<T>(arg, w);
	}

	template <typename T> inline precision_spec<T> set_prec(const T &arg, signed char p)
	{
		return precision_spec<T>(arg, p);
	}

	template <typename T> inline width_precision_spec<T> set_width_precision(const T &arg, signed char w, signed char p)
	{
		return width_precision_spec<T>(arg, w, p);
	}

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

		static int parse(const char *const fmt, const int first, const int len, settings &s)
		{
			int bookmark = first;

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

			// consume more flags
			if(bookmark >= len)
				return bookmark;
			if(fmt[bookmark] == 'x')
			{
				s.hex = true;
				++bookmark;
			}
			else if(fmt[bookmark] == 'X')
			{
				s.hex_upper = true;
				++bookmark;
			}
			else if(fmt[bookmark] == 'o')
			{
				s.oct = true;
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

		void reset()
		{
			zero_pad = false;
			left_justify = false;
			hex = false;
			hex_upper = false;
			oct = false;
			width = -1;
			precision = -1;
			index = -1;
		}

	private:
		static signed char consume_number(const char *const fmt, int &bookmark, const int len)
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
		bool hex;
		bool hex_upper;
		bool oct;

		signed char width;
		signed char precision;
		signed char index; // starts at 1
	};

	class writer
	{
	public:
		writer(print_target target, FILE *fp, char *const user_buffer, const int user_buffer_size)
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

		inline void write(const char *const buf, const int count)
		{
			switch(m_target)
			{
				case print_target::FILE_P:
					fwrite(buf, 1, count, m_fp);
					break;
				case print_target::BUFFER:
				{
					const int written = std::min(m_size - m_bookmark, count);

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
		int m_bookmark; // first unwritten byte
		const int m_size;
	};

	class parameter
	{
	public:
		enum class ptype : unsigned char
		{
			NONE,
			FLOAT64,
			SIGNED_INT,
			UNSIGNED_INT,
			BOOLEAN_,
			CHARACTER,
			VOID_POINTER,
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

		void init(const double d, const signed char w = -1, const signed char p = -1)
		{
			wi = w;
			pr = p;
			type = ptype::FLOAT64;
			object.f64 = d;
		}

		void init(const signed long long i, const signed char w = -1, const signed char p = -1)
		{
			wi = w;
			pr = p;
			type = ptype::SIGNED_INT;
			object.lli = i;
		}

		void init(const unsigned long long i, const signed char w = -1, const signed char p = -1)
		{
			wi = w;
			pr = p;
			type = ptype::UNSIGNED_INT;
			object.ulli = i;
		}

		void init(const bool b, const signed char w = -1, const signed char p = -1)
		{
			wi = w;
			pr = p;
			type = ptype::BOOLEAN_;
			object.b = b;
		}

		void init(const char c, const signed char w = -1, const signed char p = -1)
		{
			wi = w;
			pr = p;
			type = ptype::CHARACTER;
			object.c = c;
		}

		void init(const void *vp, const signed char w = -1, const signed char p = -1)
		{
			wi = w;
			pr = p;
			type = ptype::VOID_POINTER;
			object.vp = vp;
		}

		void init(const char *s, const signed char w = -1, const signed char p = -1)
		{
			wi = w;
			pr = p;
			type = ptype::BUFFER;
			object.cstr = s;
		}

		void init(std::string &&str, const signed char w = -1, const signed char p = -1)
		{
			wi = w;
			pr = p;
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
				case ptype::VOID_POINTER:
					convert_voidp(buffer, format);
					break;
				case ptype::CUSTOM:
					convert_custom(buffer, format);
					break;
				default:
					break;
			}
		}

	private:
		static void reverse(char *buff, int len)
		{
			int index = 0;
			int opposite = len - 1;
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

		template <typename T> static int stringify_int(char *buffer, T i)
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

			int place = 0;
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

		static int stringify_int_hex(char *buffer, unsigned long long i, bool uppercase)
		{
			int place = 0;
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

		static int stringify_int_oct(char *buffer, unsigned long long i)
		{
			int place = 0;
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
			const int written = snprintf(buf, sizeof(buf), "%.*f", pr == -1 ? (format.precision >= 0 ? format.precision : 6) : pr, object.f64);
			const int min = std::min(324, written);
			buffer.write(buf, min);
		}

		void convert_uint(writer &buffer, const settings &format) const
		{
			char string[22]; // big enough to store longest representation for decimal, hex, and octal for unsigned long long int

			int written = 0;
			if(format.hex || format.hex_upper)
			{
				written = stringify_int_hex(string, object.ulli, format.hex_upper);
			}
			else if(format.oct)
			{
				written = stringify_int_oct(string, object.ulli);
			}
			else
			{
				written = stringify_int(string, object.ulli);
			}

			int width = wi == -1 ? (format.width >= 0 ? format.width : 0) : wi;
			int max = std::max(written, width);
			const char pad = format.zero_pad ? '0' : ' ';

			if(!format.left_justify)
				for(int i = max; i > written; --i)
					buffer.write(&pad, 1);

			buffer.write(string, written);

			const char space = ' ';
			if(format.left_justify)
				for(int i = max; i > written; --i)
					buffer.write(&space, 1);
		}

		void convert_int(writer &buffer, const settings &format) const
		{
			char string[20];
			const int written = stringify_int(string, object.lli);

			int width = wi == -1 ? (format.width >= 0 ? format.width : 0) : wi;
			int max = std::max(written, width);
			const char pad = format.zero_pad ? '0' : ' ';

			if(!format.left_justify)
				for(int i = max; i > written; --i)
					buffer.write(&pad, 1);

			buffer.write(string, written);

			if(format.left_justify)
				for(int i = max; i > written; --i)
					buffer.write(&pad, 1);
		}

		void convert_voidp(writer &buffer, const settings &format) const
		{
			char buf[16];
			unsigned long long number = reinterpret_cast<uintptr_t>(object.vp);
			const int written = parameter::stringify_int_hex(buf, number, false);

			buffer.write(buf, written);
		}

		void convert_string(writer &buffer, const settings &format) const
		{
			const auto strlength = strlen(object.cstr);
			const int len = std::min(pr == -1 ? (format.precision < 0 ? strlength : format.precision) : pr, strlength);
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
			const void *vp;
			bool b;
			const char *cstr;
			char rawbuf[sizeof(std::string)];
		}object;
		signed char wi; // width
		signed char pr; // precision
	};

	constexpr bool is_literal_brace(const char *fmt, int len, int index)
	{
		return
		(index > len - 3) ?
			(false)
			: (fmt[index] == '{' && fmt[index + 1] == '{' && fmt[index + 2] == '}');
	}

	constexpr bool is_balanced(const char *fmt, int len, int index = 0, int open = 0)
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

	constexpr int find_partner(const char *fmt, int len, int index)
	{
		return
		(index >= len) ?
			(-1)
			: ((fmt[index] == '}') ?
				(index)
				: (find_partner(fmt, len, index + 1)));
	}

	constexpr int count_specifiers(const char *fmt, int len, int count = 0, int index = 0)
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

	void printer(const char *const fmt, const parameter *const params, const int pack_size, const print_target target, FILE *fp, char *userbuffer, const int userbuffer_size)
	{
		const int fmt_len = strlen(fmt);
		const int spec_count = count_specifiers(fmt, fmt_len);

		// buffering
		writer output(target, fp, userbuffer, userbuffer_size);

		// begin printing
		int bookmark = 0;
		for(int k = 0; k < spec_count; ++k)
		{
			// find the first open specifier bracket and extract the spec
			int spec_begin = bookmark;
			for(; spec_begin < fmt_len; ++spec_begin)
			{
				const char c = fmt[spec_begin];

				if(c == '{')
				{
					break;
				}
			}

			// print the "before" text
			output.write(fmt + bookmark, spec_begin - bookmark);
			if(is_literal_brace(fmt, fmt_len, spec_begin))
			{
				output.write("{", 1);
				bookmark = spec_begin + 3;
				--k;
				continue;
			}

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
			output.write(fmt + bookmark, fmt_len - bookmark);
	}

	template <typename T> std::string to_string(const T&)
	{
		return "{UNKNOWN DATA TYPE}";
	}

	template <typename T> struct is_pointer
	{
		constexpr static bool value = std::is_pointer<T>::value || std::is_member_pointer<T>::value || std::is_member_object_pointer<T>::value || std::is_member_function_pointer<T>::value || std::is_function<typename std::remove_pointer<T>::type>::value;
	};

	// dummy primary template
	template <typename T> inline void add_ptr(const typename std::enable_if<!is_pointer<T>::value, T>::type&, parameter*, int&, signed char, signed char) {}

	// catch pointers
	template <typename T> inline void add_ptr(const typename std::enable_if<is_pointer<T>::value, T>::type& vp, parameter *array, int &index, signed char w = -1, signed char p = -1)
	{
		array[index++].init((void*)vp, w, p);
	}

	template <typename T> inline void add(const T &x, parameter *array, int &index, signed char w = -1, signed char p = -1)
	{
		if(is_pointer<T>::value)
		{
			add_ptr<T>(x, array, index, w, p);
		}
		else
		{
			array[index++].init(std::move(press::to_string(x)));
		}
	}

	// add the argument to the parameter array
	inline void add(const unsigned long long x, parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const long long x, parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const char x, parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const double x, parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const char *x, parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const bool x, parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const std::string &x, parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x.c_str(), w, p); }

	// forward to another add overload
	inline void add(const unsigned long x, parameter *array, int &index, signed char w = -1, signed char p = -1) { add((unsigned long long)x, array, index, w, p); }
	inline void add(const unsigned x, parameter *array, int &index, signed char w = -1, signed char p = -1) { add((unsigned long long)x, array, index, w, p); }
	inline void add(const unsigned short x, parameter *array, int &index, signed char w = -1, signed char p = -1) { add((unsigned long long)x, array, index, w, p); }
	inline void add(const unsigned char x, parameter *array, int &index, signed char w = -1, signed char p = -1) { add((unsigned long long)x, array, index, w, p); }
	inline void add(const long x, parameter *array, int &index, signed char w = -1, signed char p = -1) { add((long long)x, array, index, w, p); }
	inline void add(const int x, parameter *array, int &index, signed char w = -1, signed char p = -1) { add((long long)x, array, index, w, p); }
	inline void add(const short x, parameter *array, int &index, signed char w = -1, signed char p = -1) { add((long long)x, array, index, w, p); }
	inline void add(const float x, parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init((double)x, w, p); }

	// runtime width and precision
	template <typename T> inline void add(const press::width_spec<T> &pw, parameter *array, int &index) { add(pw.arg, array, index, pw.value, -1); }
	template <typename T> inline void add(const press::precision_spec<T> &pp, parameter *array, int &index) { add(pp.arg, array, index, -1, pp.value); }
	template <typename T> inline void add(const press::width_precision_spec<T> &pwp, parameter *array, int &index) { add(pwp.arg, array, index, pwp.w, pwp.p); }

	// interfaces

	const int DEFAULT_AUTO_SIZE = 10;
	template <typename... Ts> inline void write_(print_target target, FILE *fp, char *userbuffer, int userbuffer_size, const char *fmt, const Ts&... ts)
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

		int index = 0;
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

	template <typename... Ts> void writeln(const char *fmt, const Ts&... ts)
	{
		write_(print_target::FILE_P, stdout, NULL, 0, fmt, ts...);
		puts("");
	}

	template <typename... Ts> void fwrite(FILE *fp, const char *fmt, const Ts&... ts)
	{
		write_(print_target::FILE_P, fp, NULL, 0, fmt, ts...);
	}

	template <typename... Ts> void fwriteln(FILE *fp, const char *fmt, const Ts&... ts)
	{
		write_(print_target::FILE_P, fp, NULL, 0, fmt, ts...);
		fputs("", fp);
	}

	template <typename... Ts> void bwrite(char *userbuffer, int userbuffer_size, const char *fmt, const Ts&... ts)
	{
		write_(print_target::BUFFER, NULL, userbuffer, userbuffer_size, fmt, ts...);
	}

	template <typename... Ts> void bwriteln(char *userbuffer, int userbuffer_size, const char *fmt, const Ts&... ts)
	{
		write_(print_target::BUFFER, NULL, userbuffer, userbuffer_size, fmt, ts...);
		if(userbuffer_size > 0)
		{
			const int len = strlen(userbuffer);
			if(len + 1 < userbuffer_size)
			{
				userbuffer[len] = '\n';
				userbuffer[len + 1] = 0;
			}
		}
	}
}

#endif // PRESS_H
