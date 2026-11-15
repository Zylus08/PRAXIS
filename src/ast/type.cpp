#include "praxis/ast/ast.hpp"
#include <climits>
#include <limits>
static_assert(CHAR_BIT == 8 && sizeof(int) == 4 && sizeof(long long) == 8);
static_assert(sizeof(float) == 4 && sizeof(double) == 8 && std::numeric_limits<float>::is_iec559 &&
              std::numeric_limits<double>::is_iec559);
namespace praxis {
bool numeric(Type t) {
    return !t.extent && !t.pointer && t.base >= Base::Bool && t.base <= Base::Double;
}
bool integral(Type t) {
    return numeric(t) && t.base <= Base::Long;
}
Type promoted(Type t) {
    t.constant = false;
    t.reference = false;
    if (t.base == Base::Bool || t.base == Base::Char)
        t.base = Base::Int;
    return t;
}
Type common(Type a, Type b) {
    a = promoted(a);
    b = promoted(b);
    return {a.base > b.base ? a.base : b.base};
}
std::string typeName(Type t) {
    if (t.base == Base::Nullptr)
        return "std::nullptr_t";
    std::string s;
    if (t.pointeeConstant)
        s += "const ";
    if (t.base == Base::Record || t.base == Base::Parameter) {
        s += t.name;
        if (!t.arguments.empty()) {
            s += "<";
            for (size_t i = 0; i < t.arguments.size(); ++i) {
                if (i)
                    s += ",";
                s += typeName(t.arguments[i]);
            }
            s += ">";
        }
    } else {
        const char *names[] = {
            "void",   "bool",        "char",           "int",     "long long",       "float",
            "double", "std::string", "string literal", "ostream", "endl manipulator", "record",
            "parameter", "std::nullptr_t", "auto"};
        s += names[static_cast<int>(t.base)];
    }
    for (int i = 0; i < t.pointer; ++i)
        s += "*";
    if (t.constant)
        s += " const";
    if (t.reference)
        s += "&";
    if (t.extent)
        s += "[" + std::to_string(t.extent) + "]";
    return s;
}
} // namespace praxis
