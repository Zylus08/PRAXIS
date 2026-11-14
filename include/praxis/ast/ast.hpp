#pragma once
#include "praxis/frontend/lexer.hpp"
#include <memory>
namespace praxis {
enum class Base { Void, Bool, Char, Int, Long, Float, Double, String, Text, Stream, Endl };
struct Type {
    Base base = Base::Void;
    bool constant = false, reference = false;
    int extent = 0;
    bool operator==(const Type &) const = default;
};
bool numeric(Type);
bool integral(Type);
Type promoted(Type);
Type common(Type, Type);
std::string typeName(Type);
struct Expr;
struct Stmt;
using E = std::shared_ptr<Expr>;
using S = std::shared_ptr<Stmt>;
struct Expr {
    enum class Kind { Literal, Name, Unary, Binary, Assign, Call, Index, Postfix, Member } kind;
    Token token;
    std::string op;
    std::vector<E> args;
    Type type;
    bool lvalue = false;
    int height = 1;
    Expr(Kind k, Token t, std::string o = "") : kind(k), token(std::move(t)), op(std::move(o)) {}
};
struct Decl {
    Token name;
    Type type;
    E init;
    std::vector<E> elements;
    bool braced = false;
};
struct Stmt {
    enum class Kind {
        Block,
        Declaration,
        Expression,
        If,
        While,
        Do,
        For,
        Return,
        Break,
        Continue,
        Empty
    } kind;
    Token token;
    std::vector<S> children;
    E expr, condition, step;
    Decl decl;
    Stmt(Kind k, Token t) : kind(k), token(std::move(t)) {}
};
struct Function {
    Token name;
    Type result;
    std::vector<Decl> params;
    S body;
};
struct Item {
    S statement;
    std::shared_ptr<Function> function;
};
struct Program {
    std::vector<Item> items;
    bool iostream = false, strings = false, script = false;
};
} // namespace praxis
