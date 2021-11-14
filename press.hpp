#ifndef PRESS_H
#define PRESS_H

#include <type_traits>
#include <memory>
#include <string>
#include <tuple>

#include <string.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>

/* PRESS printing tool

Press is a printing tool for human-readable output using printf style syntax, but with extra type
safety and convenience.

refer to README.md to read about features and interfaces

*/

#define pressfmtcheck(fmt, count) \
	static_assert(press::is_balanced(fmt, press::string_length(fmt)), "press: specifier brackets are not balanced!"); \
	static_assert(press::count_specifiers(fmt, press::string_length(fmt)) >= count, "press: too many parameters!")

#define prprint(fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::impl::write(press::PrintTarget::FILE_P, stdout, NULL, NULL, 0u, fmt, ##__VA_ARGS__)

#define prprintln(fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::println(fmt, ##__VA_ARGS__)

#define prfprint(fp, fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::impl::write(press::PrintTarget::FILE_P, fp, NULL, NULL, 0u, fmt, ##__VA_ARGS__)

#define prfprintln(fp, fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::fprintln(fp, fmt, ##__VA_ARGS__)

#define prbprint(userbuffer, size, fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::impl::write(press::PrintTarget::BUFFER, NULL, NULL, userbuffer, size, fmt, ##__VA_ARGS__)

#define prbprintln(userbuffer, size, fmt, ...) \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value); \
	press::bprintln(userbuffer, size, fmt, ##__VA_ARGS__)

#define prsprint(fmt, ...) \
	press::sprint(fmt, ##__VA_ARGS__); \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value);

#define prsprintln(fmt, ...) \
	press::sprintln(fmt, ##__VA_ARGS__); \
	pressfmtcheck(fmt, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value);

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

	enum class PrintTarget
	{
		FILE_P,
		STDSTRING,
		BUFFER
	};

	struct Format
	{
		Format()
		{
			reset();
		}

		static int parse(const char *const fmtstring, const int first, const int len, Format &cfg)
		{
			int bookmark = first;

			// consume sign flag
			if(bookmark >= len)
				return bookmark;
			if(fmtstring[bookmark] == ' ')
			{
				cfg.leading_space = true;
				++bookmark;
			}

			// consume separator flags
			if(bookmark >= len)
				return bookmark;
			if(fmtstring[bookmark] == ',')
			{
				const char sep = *localeconv()->decimal_point;
				cfg.thousands_sep = sep == '.' ? ',' : (sep == ',' ? '.' : ',');
				++bookmark;
			}

			// consume padding flags
			if(bookmark >= len)
				return bookmark;
			if(fmtstring[bookmark] == '0')
			{
				if(cfg.thousands_sep == 0)
					cfg.zero_pad = true;
				++bookmark;
			}
			else if(fmtstring[bookmark] == '-')
			{
				cfg.left_justify = true;
				++bookmark;
			}

			// consume representation flags
			if(bookmark >= len)
				return bookmark;
			if(fmtstring[bookmark] == 'x')
			{
				cfg.hex = true;
				++bookmark;
			}
			else if(fmtstring[bookmark] == 'X')
			{
				cfg.hex_upper = true;
				++bookmark;
			}
			else if(fmtstring[bookmark] == 'o')
			{
				cfg.oct = true;
				++bookmark;
			}

			// consume width
			if(bookmark >= len)
				return bookmark;
			cfg.width = consume_number(fmtstring, bookmark, len);

			// consume precision
			if(bookmark >= len)
				return bookmark;
			if(fmtstring[bookmark] == '.')
			{
				++bookmark;
				cfg.precision = consume_number(fmtstring, bookmark, len);
			}

			// consume index
			if(bookmark >= len)
				return bookmark;
			if(fmtstring[bookmark] == '@')
			{
				++bookmark;
				cfg.index = consume_number(fmtstring, bookmark, len);
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
			leading_space = false;
			width = -1;
			precision = -1;
			index = -1;
			thousands_sep = 0;
		}

	private:
		static signed char consume_number(const char *const fmt, int &bookmark, const int len)
		{
			unsigned char number = 0;
			bool found = false;

			while(bookmark < len && fmt[bookmark] >= '0' && fmt[bookmark] <= '9')
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
		bool leading_space;

		signed char width;
		signed char precision;
		signed char index; // starts at 1
		char thousands_sep;
	};

	class Writer
	{
	public:
		static constexpr int WRITER_BUFFER_SIZE = 1024;

		Writer(PrintTarget target, FILE *fp, std::string *stdstr, char *const user_buffer, const int user_buffer_size)
			: m_target(target)
			, m_fp(fp)
			, m_buffer(user_buffer == NULL ? m_automatic_buffer : user_buffer)
			, m_stdstring(stdstr)
			, m_bookmark(0)
			, m_size(user_buffer == NULL ? WRITER_BUFFER_SIZE : user_buffer_size)
		{}
		Writer(const Writer&) = delete;
		Writer(Writer&&) = delete;
		~Writer()
		{
			if(m_target == PrintTarget::BUFFER && m_size > 0)
				m_buffer[m_bookmark >= m_size ? m_size - 1 : m_bookmark] = 0;
			else
				flush();
		}

		inline void write(const char *const buf, const int count)
		{
			const int written = std::min(m_size - m_bookmark, count);

			memcpy(m_buffer + m_bookmark, buf, written);
			m_bookmark += written;

			if(written < count && flush())
				write(buf + written, count - written);
		}

	private:
		inline bool flush()
		{
			if(m_target == PrintTarget::BUFFER)
				return false;
			else if(m_target == PrintTarget::STDSTRING)
			{
				m_buffer[m_bookmark] = 0;
				m_stdstring->append(m_buffer);
			}
			else
				fwrite(m_buffer, 1, m_bookmark, m_fp);

			m_bookmark = 0;
			return true;
		}

		const PrintTarget m_target;
		FILE *const m_fp; // optional file pointer
		char *const m_buffer; // interface to either m_automatic_buffer or user provided buffer
		std::string *m_stdstring; // optional std::string target
		char m_automatic_buffer[WRITER_BUFFER_SIZE + 1]; // used by print_target::FILEP and STDSTRING
		int m_bookmark; // first unwritten byte
		const int m_size; // sizeof buffer pointed to by m_buffer
	};

	class Parameter
	{
	public:
		enum class Type : unsigned char
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

		Parameter() : type (Type::NONE) {}

		~Parameter()
		{
			if(type == Type::CUSTOM)
			{
				typedef std::string sstring;
				sstring *s = (sstring*)&object.raw;
				s->~sstring();
			}
		}

		void init(const double d, const signed char w = -1, const signed char p = -1)
		{
			width = w;
			precision = p;
			type = Type::FLOAT64;
			object.f64 = d;
		}

		void init(const signed long long i, const signed char w = -1, const signed char p = -1)
		{
			width = w;
			precision = p;
			type = Type::SIGNED_INT;
			object.lli = i;
		}

		void init(const unsigned long long i, const signed char w = -1, const signed char p = -1)
		{
			width = w;
			precision = p;
			type = Type::UNSIGNED_INT;
			object.ulli = i;
		}

		void init(const bool b, const signed char w = -1, const signed char p = -1)
		{
			width = w;
			precision = p;
			type = Type::BOOLEAN_;
			object.b = b;
		}

		void init(const char c, const signed char w = -1, const signed char p = -1)
		{
			width = w;
			precision = p;
			type = Type::CHARACTER;
			object.c = c;
		}

		void init(const void *vp, const signed char w = -1, const signed char p = -1)
		{
			width = w;
			precision = p;
			type = Type::VOID_POINTER;
			object.vp = vp;
		}

		void init(const char *s, const signed char w = -1, const signed char p = -1)
		{
			width = w;
			precision = p;
			type = Type::BUFFER;
			object.cstr = s;
		}

		void init(std::string &&str, const signed char w = -1, const signed char p = -1)
		{
			width = w;
			precision = p;
			type = Type::CUSTOM;
			new (&object.raw) std::string(std::move(str));
		}

		void convert(Writer &buffer, const Format &format) const
		{
			switch(type)
			{
				case Type::SIGNED_INT:
					convert_int(buffer, format);
					break;
				case Type::UNSIGNED_INT:
					convert_uint(buffer, format);
					break;
				case Type::BUFFER:
					convert_string(buffer, format);
					break;
				case Type::CHARACTER:
					convert_character(buffer, format);
					break;
				case Type::FLOAT64:
					convert_float64(buffer, format);
					break;
				case Type::BOOLEAN_:
					convert_bool(buffer, format);
					break;
				case Type::VOID_POINTER:
					convert_voidp(buffer, format);
					break;
				case Type::CUSTOM:
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

		static inline bool is_positive(unsigned long long)
		{
			return true;
		}

		static inline bool is_positive(long long i)
		{
			return i >= 0;
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
				negative = !is_positive(i);
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

		template <typename T> static void do_convert_integer(const T number, Writer &buffer, const Format &format, int runtime_width)
		{
			char string[22]; // big enough to store largest number in base 8, 10, and 16 (no terminating char needed)

			int written;
			// stringify the integer
			if(std::is_unsigned<T>::value)
			{
				if(format.hex || format.hex_upper)
					written = stringify_int_hex(string, number, format.hex_upper);
				else if(format.oct)
					written = stringify_int_oct(string, number);
				else
					written = stringify_int(string, number);
			}
			else
				written = stringify_int(string, number);

			// calculate width
			int width = runtime_width == -1 ? (format.width >= 0 ? format.width - (format.leading_space && is_positive(number)) : 0) : runtime_width;

			// calculate how many thousands separaters are needed
			int seps = 0;
			if(format.thousands_sep != 0)
			{
				seps = (written % 3) == 0 ? (written / 3) - 1 : (written / 3);
				width -= seps;
			}

			// more padding calculations
			int needed = std::max(width, written); // how many chars will actually be written
			const char pad = format.zero_pad ? '0' : ' ';

			// write a leading space if requested
			if(format.leading_space && is_positive(number))
				buffer.write(" ", 1);

			// determine if minus sign needs to be written before leading zeros
			const bool negative_and_zero_pad = !is_positive(number) && format.zero_pad;
			if(negative_and_zero_pad)
				buffer.write(string, 1);

			// apply leading pad chars
			if(!format.left_justify)
				for(int i = needed; i > written; --i)
					buffer.write(&pad, 1);

			// write the integer string
			if(seps == 0)
				buffer.write(string + (int)negative_and_zero_pad, written - (int)negative_and_zero_pad);
			else
			{
				int place = written % 3;
				if(place != 0)
				{
					buffer.write(string, place);
					buffer.write(&format.thousands_sep, 1);
				}
				for(; place < written; place += 3)
				{
					buffer.write(string + place, 3);
					if(place < written - 3)
						buffer.write(&format.thousands_sep, 1);
				}
			}

			// apply trailing pad chars
			if(format.left_justify)
				for(int i = needed; i > written; --i)
					buffer.write(&pad, 1);
		}

		void convert_float64(Writer &buffer, const Format &format) const
		{
			char buf[325];
			const int written = snprintf(buf, sizeof(buf), "%.*f", precision == -1 ? (format.precision >= 0 ? format.precision : 6) : precision, object.f64);
			const int min = std::min(324, written);
			buffer.write(buf, min);
		}

		void convert_uint(Writer &buffer, const Format &format) const
		{
			Parameter::do_convert_integer<unsigned long long>(object.ulli, buffer, format, (int)width);
		}

		void convert_int(Writer &buffer, const Format &format) const
		{
			Parameter::do_convert_integer<long long>(object.lli, buffer, format, (int)width);
		}

		void convert_voidp(Writer &buffer, const Format&) const
		{
			char buf[16];
			unsigned long long number = reinterpret_cast<uintptr_t>(object.vp);
			const int written = Parameter::stringify_int_hex(buf, number, false);

			buffer.write(buf, written);
		}

		void convert_string(Writer &buffer, const Format &format) const
		{
			const auto strlength = strlen(object.cstr);
			const int len = std::min(precision == -1 ? (format.precision < 0 ? strlength : format.precision) : precision, strlength);
			buffer.write(object.cstr, len);
		}

		void convert_bool(Writer &buffer, const Format&) const
		{
			buffer.write(object.b ? "true" : "false", object.b ? 4 : 5);
		}

		void convert_character(Writer &buffer, const Format&) const
		{
			char c = object.c;
			buffer.write(&c, 1);
		}

		void convert_custom(Writer &buffer, const Format&) const
		{
			const std::string *s = (const std::string*)&object.raw;
			buffer.write(s->c_str(), s->length());
		}

		Type type;
		union
		{
			long long lli;
			unsigned long long ulli;
			double f64;
			char c;
			const void *vp;
			bool b;
			const char *cstr;
			std::aligned_storage<sizeof(std::string), alignof(std::string)>::type raw;
		} object;
		signed char width;
		signed char precision;
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

	template <typename T> std::string to_string(const T&)
	{
		return "{UNKNOWN DATA TYPE}";
	}

	namespace impl{
	static void printer(const char *const fmt, const Parameter *const params, const int pack_size, const PrintTarget target, FILE *fp, std::string *stdstring, char *userbuffer, const int userbuffer_size)
	{
		const int fmt_len = strlen(fmt);
		const int spec_count = count_specifiers(fmt, fmt_len);

		// buffering
		Writer output(target, fp, stdstring, userbuffer, userbuffer_size);

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

			if(spec_begin >= fmt_len)
				return;

			// print the "before" text
			output.write(fmt + bookmark, spec_begin - bookmark);
			if(is_literal_brace(fmt, fmt_len, spec_begin))
			{
				output.write("{", 1);
				bookmark = spec_begin + 3;
				--k;
				continue;
			}

			Format format_settings;
			bookmark = Format::parse(fmt, spec_begin + 1, fmt_len, format_settings) + 1;

			const bool spec_index_overridden = format_settings.index >= 0;
			const int index = spec_index_overridden ? format_settings.index - 1 : k;
			if(spec_index_overridden && (index < 0 || index >= (int)pack_size))
				output.write("{UNDEFINED}", 11);
			else if(!spec_index_overridden && index >= (int)pack_size)
				output.write("{UNDEFINED}", 11);
			else
				params[index].convert(output, format_settings);
		}

		// print the "tail"
		if(bookmark < fmt_len)
		{
			// look for literal brace patterns
			for(;;)
			{
				int index = bookmark;
				bool found = false;
				while(index < fmt_len)
				{
					if(is_literal_brace(fmt, fmt_len, index))
					{
						found = true;
						break;
					}

					++index;
				}

				if(!found)
					break;

				// print the text before the brace pattern
				output.write(fmt + bookmark, index - bookmark);

				// print a literal brace
				const char brace = '{';
				output.write(&brace, 1);

				bookmark = index + 3;
			}

			// write the last little bit
			output.write(fmt + bookmark, fmt_len - bookmark);
		}
	};

	template <typename T> struct is_pointer
	{
		constexpr static bool value = std::is_pointer<T>::value || std::is_member_pointer<T>::value || std::is_member_object_pointer<T>::value || std::is_member_function_pointer<T>::value || std::is_function<typename std::remove_pointer<T>::type>::value;
	};

	// dummy primary template
	template <typename T> inline void add_ptr(const typename std::enable_if<!is_pointer<T>::value, T>::type&, Parameter*, int&, signed char, signed char) {}

	// catch pointers
	template <typename T> inline void add_ptr(const typename std::enable_if<is_pointer<T>::value, T>::type& vp, Parameter *array, int &index, signed char w = -1, signed char p = -1)
	{
		array[index++].init((void*)vp, w, p);
	}

	template <typename T> inline void add(const T &x, Parameter *array, int &index, signed char w = -1, signed char p = -1)
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
	inline void add(const unsigned long long x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const long long x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const char x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const double x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const char *x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const bool x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x, w, p); }
	inline void add(const std::string &x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init(x.c_str(), w, p); }

	// forward to another add overload
	inline void add(const unsigned long x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { add((unsigned long long)x, array, index, w, p); }
	inline void add(const unsigned x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { add((unsigned long long)x, array, index, w, p); }
	inline void add(const unsigned short x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { add((unsigned long long)x, array, index, w, p); }
	inline void add(const unsigned char x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { add((unsigned long long)x, array, index, w, p); }
	inline void add(const long x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { add((long long)x, array, index, w, p); }
	inline void add(const int x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { add((long long)x, array, index, w, p); }
	inline void add(const short x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { add((long long)x, array, index, w, p); }
	inline void add(const float x, Parameter *array, int &index, signed char w = -1, signed char p = -1) { array[index++].init((double)x, w, p); }

	// runtime width and precision
	template <typename T> inline void add(const press::width_spec<T> &pw, Parameter *array, int &index) { add(pw.arg, array, index, pw.value, -1); }
	template <typename T> inline void add(const press::precision_spec<T> &pp, Parameter *array, int &index) { add(pp.arg, array, index, -1, pp.value); }
	template <typename T> inline void add(const press::width_precision_spec<T> &pwp, Parameter *array, int &index) { add(pwp.arg, array, index, pwp.w, pwp.p); }

	// interfaces

	const int DEFAULT_AUTO_SIZE = 10;
	template <typename... Ts> inline void write(PrintTarget target, FILE *fp, std::string *stdstring, char *userbuffer, int userbuffer_size, const char *fmt, const Ts&... ts)
	{
		Parameter *storage;
		std::unique_ptr<Parameter[]> dynamic;
		Parameter automatic[DEFAULT_AUTO_SIZE];
		if(sizeof...(Ts) > DEFAULT_AUTO_SIZE)
		{
			dynamic.reset(new Parameter[sizeof...(Ts)]);
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

		impl::printer(fmt, storage, sizeof...(Ts), target, fp, stdstring, userbuffer, userbuffer_size);
	}}

	template <typename... Ts> void print(const char *fmt, const Ts&... ts)
	{
		impl::write(PrintTarget::FILE_P, stdout, NULL, NULL, 0, fmt, ts...);
	}

	template <typename... Ts> void println(const char *fmt, const Ts&... ts)
	{
		impl::write(PrintTarget::FILE_P, stdout, NULL, NULL, 0, fmt, ts...);
		const char newline = '\n';
		fwrite(&newline, 1, 1, stdout);
	}

	template <typename... Ts> void fprint(FILE *fp, const char *fmt, const Ts&... ts)
	{
		impl::write(PrintTarget::FILE_P, fp, NULL, NULL, 0, fmt, ts...);
	}

	template <typename... Ts> void fprintln(FILE *fp, const char *fmt, const Ts&... ts)
	{
		impl::write(PrintTarget::FILE_P, fp, NULL, NULL, 0, fmt, ts...);
		const char newline = '\n';
		fwrite(&newline, 1, 1, fp);
	}

	template <typename... Ts> void bprint(char *userbuffer, int userbuffer_size, const char *fmt, const Ts&... ts)
	{
		impl::write(PrintTarget::BUFFER, NULL, NULL, userbuffer, userbuffer_size, fmt, ts...);
	}

	template <typename... Ts> void bprintln(char *userbuffer, int userbuffer_size, const char *fmt, const Ts&... ts)
	{
		impl::write(PrintTarget::BUFFER, NULL, NULL, userbuffer, userbuffer_size, fmt, ts...);
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

	template <typename... Ts> std::string sprint(const char *fmt, const Ts&... ts)
	{
		std::string output;
		impl::write(PrintTarget::STDSTRING, NULL, &output, NULL, 0, fmt, ts...);

		return output;
	}

	template <typename... Ts> std::string sprintln(const char *fmt, const Ts&... ts)
	{
		std::string output;
		impl::write(PrintTarget::STDSTRING, NULL, &output, NULL, 0, fmt, ts...);
		output.push_back('\n');

		return output;
	}
}

#endif // PRESS_H
