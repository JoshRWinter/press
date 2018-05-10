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
		}

		void reset(){ zero_pad = false; left_justify = false; width = 0; precision = 0; index = -1; }

		// flags
		bool zero_pad;
		bool left_justify;

		unsigned width;
		unsigned precision;
		int index; // starts at 1
	};

	class parameter
	{
	public:
		enum class ptype
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

		const char *convert(char *buffer, unsigned size, const settings &format) const
		{
			switch(type)
			{
				case ptype::SIGNED_INT:
					return convert_int(buffer, size, format);
				case ptype::UNSIGNED_INT:
					return convert_uint(buffer, size, format);
				case ptype::STRING:
					return (const char*)object;
				case ptype::FLOAT32:
					return convert_float32(buffer, size, format);
				case ptype::CHARACTER:
					return convert_uint(buffer, size, format);
				case ptype::FLOAT64:
					return convert_float64(buffer, size, format);
				case ptype::BOOLEAN_:
					return convert_bool(buffer, size, format);
				case ptype::POINTER:
					return convert_pointer(buffer, size, format);
				default:
					abort("unrecognized enum case: " + std::to_string((int)type));
					break;
			}

			return NULL;
		}

	private:
		const char *convert_float64(char *buffer, unsigned size, const settings &format) const
		{
			snprintf(buffer, size, "double with zp=%s, lj=%s, width=%u, precs=%u, index=%i", format.zero_pad ? "true" : "false", format.left_justify ? "true" : "false", format.width, format.precision, format.index);
			return buffer;
		}

		const char *convert_float32(char *buffer, unsigned size, const settings &format) const
		{
			snprintf(buffer, size, "float with zp=%s, lj=%s, width=%u, precs=%u, index=%i", format.zero_pad ? "true" : "false", format.left_justify ? "true" : "false", format.width, format.precision, format.index);
			return buffer;
		}

		const char *convert_int(char *buffer, unsigned size, const settings &format) const
		{
			snprintf(buffer, size, "int with zp=%s, lj=%s, width=%u, precs=%u, index=%i", format.zero_pad ? "true" : "false", format.left_justify ? "true" : "false", format.width, format.precision, format.index);
			return buffer;
		}

		const char *convert_uint(char *buffer, unsigned size, const settings &format) const
		{
			snprintf(buffer, size, "uint with zp=%s, lj=%s, width=%u, precs=%u, index=%i", format.zero_pad ? "true" : "false", format.left_justify ? "true" : "false", format.width, format.precision, format.index);
			return buffer;
		}

		const char *convert_bool(char *buffer, unsigned size, const settings &format) const
		{
			snprintf(buffer, size, "bool with zp=%s, lj=%s, width=%u, precs=%u, index=%i", format.zero_pad ? "true" : "false", format.left_justify ? "true" : "false", format.width, format.precision, format.index);
			return buffer;
		}

		const char *convert_character(char *buffer, unsigned size, const settings &format) const
		{
			snprintf(buffer, size, "char with zp=%s, lj=%s, width=%u, precs=%u, index=%i", format.zero_pad ? "true" : "false", format.left_justify ? "true" : "false", format.width, format.precision, format.index);
			return buffer;
		}

		const char *convert_pointer(char *buffer, unsigned size, const settings &format) const
		{
			snprintf(buffer, size, "pointer with zp=%s, lj=%s, width=%u, precs=%u, index=%i", format.zero_pad ? "true" : "false", format.left_justify ? "true" : "false", format.width, format.precision, format.index);
			return buffer;
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

	void print_plain(const char *fmt, unsigned start, unsigned spec_begin)
	{
		const char *const substr = fmt + start;
		const char *const position = strstr(substr, "{{}");
		const bool found = position != NULL && position < fmt + spec_begin;
		const unsigned index = found == false ? -1 : (position - fmt);
		const unsigned len = found == false ? spec_begin - start : (index - start);

		if(found == false)
			printf("%.*s", len, substr);
		else
		{
			printf("%.*s", len + 1, substr);
			print_plain(fmt, index + 3, spec_begin);
		}
	}

	void printer(const char *const fmt, const parameter *const params, const unsigned param_count, bool checked)
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

		// conversion buffer
		char conversions[501];

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
			print_plain(fmt, bookmark, spec_begin);

			settings format_settings(fmt, spec_begin + 1, spec_len);
			const char *const buf = params[k].convert(conversions, sizeof(conversions), format_settings);
			printf("{%s}", buf);

			bookmark = spec_end + 1;
		}

		if(bookmark < fmt_len)
			print_plain(fmt, bookmark, fmt_len);
	}

	// fallback
	template <typename T> inline void add(const T&, parameter*, unsigned&) { }

	typedef const char* constchar_p;
	// meaningfull specializations
	template <> inline void add(const unsigned long long &x, parameter *array, unsigned &index) { array[index++].init(x); }
	template <> inline void add(const long long &x, parameter *array, unsigned &index) { array[index++].init(x); }
	template <> inline void add(const char &x, parameter *array, unsigned &index) { array[index++].init(x); }
	template <> inline void add(const double &x, parameter *array, unsigned &index) { array[index++].init(x); }
	template <> inline void add(const float &x, parameter *array, unsigned &index) { array[index++].init(x); }
	template <> inline void add(const constchar_p &x, parameter *array, unsigned &index) { array[index++].init(x); }
	template <> inline void add(const bool &x, parameter *array, unsigned &index) { array[index++].init(x); }

	// specializations that delegate to other specializations
	template <> inline void add(const unsigned long &x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	template <> inline void add(const unsigned &x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	template <> inline void add(const unsigned short &x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	template <> inline void add(const unsigned char &x, parameter *array, unsigned &index) { add((unsigned long long)x, array, index); }
	template <> inline void add(const signed long &x, parameter *array, unsigned &index) { add((long long)x, array, index); }
	template <> inline void add(const int &x, parameter *array, unsigned &index) { add((long long)x, array, index); }
	template <> inline void add(const short &x, parameter *array, unsigned &index) { add((long long)x, array, index); }

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

		unsigned index = 0;
		const char dummy[sizeof...(Ts)] = { (add(ts, storage, index), (char)1)... };

		#if defined (__GNUC__)
		#pragma GCC diagnostic pop
		#endif

		printer(fmt, storage, sizeof...(Ts), true);
	}
}

#endif // PRESS_H
