#pragma once
#include "praxis/ast/ast.hpp"
#include <variant>
#include <map>
#include <cstdint>
namespace praxis {
struct Object;
struct Pointer {
    std::uint64_t targetId = 0;
    std::uint64_t generation = 0;
    std::shared_ptr<Object> target;
    int offset = 0;
    bool isNull = true;
    bool operator==(const Pointer &other) const {
        if (isNull && other.isNull) return true;
        return isNull == other.isNull && targetId == other.targetId &&
               generation == other.generation && target == other.target && offset == other.offset;
    }
};
struct Value {
    Type type;
    std::variant<std::int64_t, double, std::string, std::shared_ptr<Object>, Pointer> data = std::int64_t{0};
};
struct Object {
    Type type;
    Value value;
    bool initialized = false, alive = true;
    std::vector<std::shared_ptr<Object>> elements;
    std::map<std::string,std::shared_ptr<Object>> fields;
    std::vector<std::shared_ptr<Object>> members;
    std::shared_ptr<Class> definition;
    bool complete=false, destroying=false, zeroInitialize=false;
    Location location;
    std::uint64_t id = 0;
    std::uint64_t generation = 1;
    bool isHeap = false;
    bool deleted = false;
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
