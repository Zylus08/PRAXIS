#pragma once
#include "praxis/ast/ast.hpp"
#include <set>
namespace praxis {
class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;
    int depth = 0;
    std::set<std::string> knownTypes, templateTypes;
    std::vector<Type> typeArguments();
    std::vector<std::string> templateParameters();
    std::vector<Decl> parameters();
    std::shared_ptr<Function> function(Type,Token,std::string="");
    std::shared_ptr<Class> classDeclaration();
    void functionBody(const std::shared_ptr<Function>&);
    bool functionAhead() const;
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
    explicit Parser(std::vector<Token> t, std::set<std::string> names={}) : tokens(std::move(t)),knownTypes(std::move(names)) {}
    Program parse(bool script = false);
};
} // namespace praxis
