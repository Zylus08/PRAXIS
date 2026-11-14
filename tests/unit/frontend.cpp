#include "praxis/frontend/lexer.hpp"
#include "praxis/frontend/parser.hpp"
#include "praxis/semantics/analyzer.hpp"
#include <iostream>
int main() {
    using namespace praxis;
    int checks = 0;
    auto test = [&](bool x, const char *n) {
        ++checks;
        if (!x) {
            std::cerr << n << '\n';
            throw std::runtime_error(n);
        }
    };
    auto t = lex(
        {"unit.cpp",
         "int x = 42;\n // comment\n x += 1.5f; /* multi\n line */ char c='\\n'; \"hi\\t\" true"});
    test(t[0].kind == TokenKind::Keyword, "keyword");
    test(t[1].kind == TokenKind::Identifier, "identifier");
    test(t[3].kind == TokenKind::Integer, "integer");
    test(t[3].text == "42", "integer spelling");
    test(t[5].loc.line == 3, "line");
    test(t[5].loc.column == 2, "column");
    test(t[6].text == "+=", "maximal munch");
    test(t[7].kind == TokenKind::Floating, "float");
    test(t[12].text == "\n", "character escape");
    test(t[14].text == "hi\t", "string escape");
    test(t.back().kind == TokenKind::End, "end token");
    test(t[10].loc.file == "unit.cpp", "filename");
    auto p = Parser(lex({"precedence", "int main(){return 2+3*4;}"})).parse();
    Analyzer{}.check(p);
    auto e = p.items[0].function->body->children[0]->expr;
    test(e->op == "+", "addition root");
    test(e->args[1]->op == "*", "multiplication binds");
    test(e->type.base == Base::Int, "typed expression");
    std::cout << checks << " frontend checks passed\n";
}
