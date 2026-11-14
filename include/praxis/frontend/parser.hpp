#pragma once
#include "praxis/ast/ast.hpp"
namespace praxis {
class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;
    int depth = 0;
    const Token &peek(size_t = 0) const;
    Token take();
    bool match(const std::string &);
    Token expect(const std::string &);
    Token identifier();
    bool typeStart() const;
    Type type();
    Decl declaration(Type, Token);
    S statement();
    S block();
    E expression(int = 1);
    E prefix();
    E postfix(E);
    int precedence(const std::string &) const;

  public:
    explicit Parser(std::vector<Token> t) : tokens(std::move(t)) {}
    Program parse(bool script = false);
};
} // namespace praxis
