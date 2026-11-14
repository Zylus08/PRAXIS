#pragma once
#include "praxis/ast/ast.hpp"
#include <variant>
namespace praxis {
struct Value {
    Type type;
    std::variant<std::int64_t, double, std::string> data = std::int64_t{0};
};
struct Object {
    Type type;
    Value value;
    bool initialized = false, alive = true;
    std::vector<std::shared_ptr<Object>> elements;
};
struct Evaluation {
    Value value;
    std::shared_ptr<Object> object;
};
Value literal(const Expr &);
Value zero(Type);
Value convertValue(Value, Type, const Location &);
bool truth(const Value &);
Value binary(const std::string &, Value, Value, const Location &);
Value read(const Evaluation &, const Location &);
} // namespace praxis
