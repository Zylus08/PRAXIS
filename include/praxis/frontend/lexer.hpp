#pragma once
#include "praxis/common/source.hpp"
namespace praxis {
enum class TokenKind { Identifier, Keyword, Integer, Floating, String, Character, Symbol, End };
struct Token {
    TokenKind kind;
    std::string text;
    Location loc;
    bool iostreamAvailable = false, stringAvailable = false;
    bool is(const std::string &s) const {
        return (kind == TokenKind::Symbol || kind == TokenKind::Keyword ||
                kind == TokenKind::Identifier) &&
               text == s;
    }
};
std::vector<Token> lex(const Source &);
} // namespace praxis
