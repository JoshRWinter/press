PRESS printing tool

Press is a printing tool for human-readable output using printf style syntax, but with extra type safety and convenience.

# FEATURES:
- User-defined types
- Positional specifiers
- Runtime width and precision
- Small implementation (less than 1k lines), in a single header file
- Fast, also makes 0 memory allocations
	- Except for the std::string returned from your overloaded function for a custom type
	- If more than 16 parameters are passed to the printing function, a memory allocation must be made to accomodate all of the parameters

# How to use
- Simply include this header "press.h" and make sure to compile your project with at least c++11
- Formatting specifiers are "{}" with optional flags inside the brackets
- To use press with a custom type, simply overload the `std::string press::to_string(const Myclass&)` function, taking a const-reference to your class, and return a std::string

## Formatting parameters
Optional formatting parameters are accepted inside the {} brackets IN THIS ORDER:

1) Sign flag: optional `' '` (space) when positive signed integers should be printed with a leading space

2) Separator flag: optional `,` (comma) for thousands separators as defined by your locale

3) Padding flags: zero or one of the following symbols to control how padding is applied  
	`0`	The integer parameter should be zero padded, if padding is to be applied  
	`-` (dash) The integer parameter should be left-justified

4) Representation flags: zero or one of the following symbols to control representation, for unsigned integers  
	`x`	The unsigned integer parameter should be displayed in base 16  
	`X`	Same as above, but with uppercase ABCDEF  
	`o` (oh) The unsigned integer parameter should be displayed in base 8

5) An optional width parameter (positive integer), that specifies the minimum number of characters to be printed for integers

6) An optional precision parameter (positive integer), preceded with a . (dot), that specifies the number of digits after the decimal for floats, and the number of characters to be printed for a string

7) An optional positional specifier (positive non-zero integer), preceded with an @ (at sign)

## Runtime width and precision
Instead of specifying width and/or precision in the format string, you may specify at runtime.  
Runtime-specified width and/or precision overrides any specification in the format string  
Surround the parameter with a call to  
`press::set_width(param, width)`  
`press::set_prec(param, precision)`  
`press::set_width_precision(param, width, precision)`  
E.G. `press::write("this integer has runtime specified width: {}\n", press::set_width(myinteger, 2))`
