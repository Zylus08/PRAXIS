# M1 language boundary

## Implemented

- Decimal integers (optional `LL`/`ll`), decimal floating literals with exponents (optional `f`/`F`), byte character and string literals, `true`/`false`.
- `//` and `/* */` comments; source file, line, and byte column diagnostics. Escapes: `\n`, `\r`, `\t`, `\0`, `\\`, `\'`, `\"`.
- `bool`, signed 8-bit `char`, 32-bit `int`, 64-bit `long long`, binary32 `float`, binary64 `double`, `void` function results, `std::string`.
- All C++20 keywords are reserved; `std` is reserved for native bindings.
- One declarator per declaration, optional top-level `const`, assignment initialization. Both `const int` and `int const` spelling.
- Fixed one-dimensional arrays with literal decimal bounds 1–1,000,000. Brace initialization, trailing comma, zero-filling omitted elements, indexing, element references. No array decay or array parameters.
- Lvalue references to matching scalar/string objects, including const references to existing lvalues. Local aliases and reference parameters preserve identity. No reference returns or binding const references to temporaries.
- Named function prototypes/definitions, parameters, return conversion, recursion, forward declarations. No overloads. Prototype parameters require names. `int main()` or `int main(void)` is required in file mode.
- Blocks, if/else, while, do/while, for, return, break, continue, empty statements.
- Arithmetic `+ - * / %`, comparison `== != < > <= >=`, short-circuit `&& ||`, unary `+ - !`, assignment and `+= -= *= /= %=`, prefix/postfix `++ --`, parentheses, calls, indexing.
- Right-associative assignment; ordinary C++ precedence for implemented operators; dangling else binds to the nearest if.
- Object-like `#define`, `#undef`, quoted and bracketed `#include`, recursive object-macro expansion with suppression of recursive re-expansion, `-I` include directories, include-cycle detection.

## Native runtime contracts

`#include <iostream>` exposes `std::cout` and the `std::endl` manipulator. Insert arithmetic values, byte characters, string literals, or `std::string` values using `<<`. Bool prints `0`/`1`; char prints a byte; default numeric formatting is delegated to the host C++ stream with its default formatting/locale. `std::endl` emits newline and flushes. No other stream objects, formatting flags, extraction, files, or manipulators exist. Names are unavailable before their include.

`#include <string>` exposes `std::string`: default construction, literal initialization, copies, assignment, string/literal concatenation and comparison, string/char concatenation, `+=` for strings/literals/chars, `size()`, `length()`, `empty()`. Storage and these operations are explicitly bridged to host `std::string`. Literal conversion/stream insertion observes C-string NUL termination. String members return signed `long long` lengths in M1, not the full C++ `size_t` type. No string indexing, iterators, allocator exposure, or arbitrary member calls.

A string literal is a restricted read-only text expression, only usable in native stream insertion and supported string conversions/operators. M1 does not expose its standard array type, pointer decay, address, or pointer comparison.

## Intentional rejection

Classes, structs, members outside the documented string binding, constructors/destructors, templates, overloads, user namespaces, using directives, pointers, casts, `auto`, enum, union, typedef, exceptions, lambdas, dynamic allocation, unsigned types, `short`, single `long`, wide characters, raw/adjacent string literals, hexadecimal/octal/binary literals, bitwise operators, integer shifts, ternary/comma operators, switch, goto, static locals, extern, aggregate scalar initialization, multiple declarators, variable-length/multidimensional arrays, and general STL headers are not supported.

Full preprocessing is absent: conditional directives, guards, pragma, function-like macros, token paste/stringification, line splicing, include operands produced by macros, and predefined compiler macros are rejected or outside the documented syntax. Repeating the virtual native headers is allowed; project headers have no implicit include-once behavior. Macro redefinition requires explicit `#undef`.
