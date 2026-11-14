#include "praxis/frontend/lexer.hpp"
#include <cctype>
#include <unordered_set>
namespace praxis {
std::vector<Token> lex(const Source &s) {
    std::vector<Token> out;
    size_t i = 0;
    Location l{s.name};
    auto peek = [&](size_t n = 0) { return i + n < s.text.size() ? s.text[i + n] : '\0'; };
    auto take = [&]() {
        char c = s.text[i++];
        if (c == '\n') {
            ++l.line;
            l.column = 1;
        } else
            ++l.column;
        return c;
    };
    auto alpha = [](char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; };
    auto digit = [](char c) { return std::isdigit(static_cast<unsigned char>(c)); };
    const std::unordered_set<std::string> keywords = {
        "alignas",       "alignof",     "and",
        "and_eq",        "asm",         "auto",
        "bitand",        "bitor",       "bool",
        "break",         "case",        "catch",
        "char",          "char8_t",     "char16_t",
        "char32_t",      "class",       "compl",
        "concept",       "const",       "consteval",
        "constexpr",     "constinit",   "const_cast",
        "continue",      "co_await",    "co_return",
        "co_yield",      "decltype",    "default",
        "delete",        "do",          "double",
        "dynamic_cast",  "else",        "enum",
        "explicit",      "export",      "extern",
        "false",         "float",       "for",
        "friend",        "goto",        "if",
        "inline",        "int",         "long",
        "mutable",       "namespace",   "new",
        "noexcept",      "not",         "not_eq",
        "nullptr",       "operator",    "or",
        "or_eq",         "private",     "protected",
        "public",        "register",    "reinterpret_cast",
        "requires",      "return",      "short",
        "signed",        "sizeof",      "static",
        "static_assert", "static_cast", "struct",
        "switch",        "template",    "this",
        "thread_local",  "throw",       "true",
        "try",           "typedef",     "typeid",
        "typename",      "union",       "unsigned",
        "using",         "virtual",     "void",
        "volatile",      "wchar_t",     "while",
        "xor",           "xor_eq"};
    while (i < s.text.size()) {
        if (std::isspace(static_cast<unsigned char>(peek()))) {
            take();
            continue;
        }
        if (peek() == '/' && peek(1) == '/') {
            while (peek() && peek() != '\n')
                take();
            continue;
        }
        if (peek() == '/' && peek(1) == '*') {
            auto start = l;
            take();
            take();
            while (peek() && !(peek() == '*' && peek(1) == '/'))
                take();
            if (!peek())
                fail(start, "lex", "unterminated comment");
            take();
            take();
            continue;
        }
        auto start = l;
        std::string t;
        TokenKind k = TokenKind::Symbol;
        if (alpha(peek())) {
            while (alpha(peek()) || digit(peek()))
                t += take();
            k = keywords.contains(t) ? TokenKind::Keyword : TokenKind::Identifier;
        } else if (digit(peek()) || (peek() == '.' && digit(peek(1)))) {
            k = TokenKind::Integer;
            while (digit(peek()))
                t += take();
            if (peek() == '.') {
                k = TokenKind::Floating;
                t += take();
                while (digit(peek()))
                    t += take();
            }
            if (peek() == 'e' || peek() == 'E') {
                k = TokenKind::Floating;
                t += take();
                if (peek() == '+' || peek() == '-')
                    t += take();
                if (!digit(peek()))
                    fail(start, "lex", "expected exponent digits");
                while (digit(peek()))
                    t += take();
            }
            if (k == TokenKind::Floating && (peek() == 'f' || peek() == 'F'))
                t += take();
            else if (k == TokenKind::Integer && (peek() == 'l' || peek() == 'L')) {
                char c = take();
                t += c;
                if (peek() != c)
                    fail(start, "lex", "only LL integer suffix is supported");
                t += take();
            }
            if (alpha(peek()) || digit(peek()))
                fail(start, "lex", "unsupported numeric literal");
            if (k == TokenKind::Integer && t.size() > 1 && t[0] == '0' && digit(t[1]))
                fail(start, "lex", "octal literals are unsupported");
        } else if (peek() == '"' || peek() == '\'') {
            char quote = take();
            k = quote == '"' ? TokenKind::String : TokenKind::Character;
            while (peek() && peek() != quote) {
                if (peek() == '\n' || peek() == '\r')
                    fail(start, "lex", "newline in literal");
                char c = take();
                if (c == '\\') {
                    if (!peek())
                        fail(start, "lex", "unterminated escape");
                    c = take();
                    switch (c) {
                    case 'n':
                        c = '\n';
                        break;
                    case 'r':
                        c = '\r';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    case '0':
                        c = '\0';
                        break;
                    case '\\':
                    case '\'':
                    case '"':
                        break;
                    default:
                        fail(start, "lex", "unsupported escape");
                    }
                }
                t += c;
            }
            if (!peek())
                fail(start, "lex", "unterminated literal");
            take();
            if (k == TokenKind::Character && t.size() != 1)
                fail(start, "lex", "character literal must contain one byte");
        } else {
            static const std::unordered_set<std::string> pairs = {
                "++", "--", "+=", "-=", "*=", "/=", "%=", "==",
                "!=", "<=", ">=", "&&", "||", "<<", ">>", "::"};
            t += take();
            std::string pair = t + peek();
            if (pairs.contains(pair))
                t += take();
            else if (std::string("+-*/%=!<>&|;:,(){}[].#~").find(t[0]) == std::string::npos)
                fail(start, "lex", "unsupported character");
        }
        out.push_back({k, t, start});
    }
    out.push_back({TokenKind::End, "", l});
    return out;
}
} // namespace praxis
