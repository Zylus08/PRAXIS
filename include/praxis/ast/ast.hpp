#pragma once
#include "praxis/frontend/lexer.hpp"
#include <memory>
#include <map>
namespace praxis {
enum class Base { Void, Bool, Char, Int, Long, Float, Double, String, Text, Stream, Endl, Record, Parameter, Nullptr, Auto };
struct Type {
    Base base = Base::Void;
    bool constant = false, reference = false;
    int pointer = 0;
    bool pointeeConstant = false;
    int extent = 0;
    std::string name;
    std::vector<Type> arguments;
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
    enum class Kind { Literal, Name, Unary, Binary, Assign, Call, Index, Postfix, Member, Construct, New, Delete, InitList } kind;
    Token token;
    std::string op;
    std::vector<E> args;
    Type type;
    bool lvalue = false;
    int height = 1;
    std::string target, copyTarget;
    bool implicitMember=false, parenthesized=false;
    std::vector<Type> typeArguments;
    Expr(Kind k, Token t, std::string o = "") : kind(k), token(std::move(t)), op(std::move(o)) {}
};
struct Decl {
    Token name;
    Type type;
    E init;
    std::vector<E> elements;
    bool braced = false;
    bool direct=false, publicAccess=true;
    std::vector<E> arguments;
    std::string constructor;
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
        Empty,
        RangeFor
    } kind;
    Token token;
    std::vector<S> children;
    E expr, condition, step;
    Decl decl;
    Stmt(Kind k, Token t) : kind(k), token(std::move(t)) {}
};
struct MemberInit { Token name; std::vector<E> args; std::string constructor; bool valueInitialize=false; };
struct Function {
    Token name;
    Type result;
    std::vector<Decl> params;
    S body;
    std::string owner, id;
    bool constMember=false, constructor=false, destructor=false, publicAccess=true;
    enum class Special { User, DefaultConstructor, CopyConstructor } special=Special::User;
    std::vector<MemberInit> initializers;
    std::vector<std::string> templateParameters;
    bool instantiated=false;
};
struct Class {
    Token name;
    std::string key;
    std::vector<Decl> fields;
    std::vector<std::shared_ptr<Function>> methods;
    std::vector<std::string> templateParameters;
    Type type;
    std::string copyConstructor, destructor;
    bool assignable=true;
    size_t nestingDepth=1;
};
struct Item {
    S statement;
    std::shared_ptr<Function> function;
    std::shared_ptr<Class> record;
};
struct Program {
    std::vector<Item> items;
    bool iostream = false, strings = false, script = false;
    bool vector = false, array = false, utility = false, algorithm = false;
    std::map<std::string,std::shared_ptr<Function>> compiledFunctions;
    std::map<std::string,std::shared_ptr<Class>> compiledClasses;
    size_t instantiations=0, cacheHits=0;
    std::uint64_t templateNanoseconds=0;
};
struct Limits { size_t calls=128, templateDepth=32, instantiations=256, objects=100000; std::uint64_t steps=10000000; };
struct Profile { std::uint64_t objects=0,calls=0,destructors=0,constructions=0,copies=0,assignments=0; size_t peakObjects=0; std::uint64_t callNanoseconds=0,constructorNanoseconds=0,destructorNanoseconds=0;
    std::uint64_t dynamicAllocations=0, dynamicDeletions=0, containerOperations=0;
};
} // namespace praxis
