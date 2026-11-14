#pragma once
#include "praxis/ast/ast.hpp"
#include <map>
#include <set>
namespace praxis {
class Analyzer {
    std::vector<std::map<std::string, Type>> scopes;
    std::map<std::string, std::shared_ptr<Function>> functions;
    std::set<std::string> definitions, called;
    Program *program = nullptr;
    Type result;
    int loops = 0;
    bool inFunction = false;
    Type expr(const E &);
    void stmt(const S &, bool = true);
    void declare(Decl &);
    void bind(const Token &, Type);
    Type lookup(const Token &);
    void convert(const E &, Type);
    void condition(const E &);
    void modifiable(const E &);
    bool returns(const S &) const;

  public:
    void check(Program &);
};
} // namespace praxis
