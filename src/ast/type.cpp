#include "praxis/ast/ast.hpp"
#include <climits>
#include <limits>
static_assert(CHAR_BIT == 8 && sizeof(int) == 4 && sizeof(long long) == 8);
static_assert(sizeof(float) == 4 && sizeof(double) == 8 && std::numeric_limits<float>::is_iec559 &&
              std::numeric_limits<double>::is_iec559);
namespace praxis {
bool numeric(Type t) {
    return !t.extent && t.base >= Base::Bool && t.base <= Base::Double;
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
    if(t.base==Base::Record || t.base==Base::Parameter){std::string n=t.name;if(!t.arguments.empty()){n+="<";for(size_t i=0;i<t.arguments.size();++i){if(i)n+=",";n+=typeName(t.arguments[i]);}n+=">";}return std::string(t.constant?"const ":"")+n+(t.reference?"&":"")+(t.extent?"["+std::to_string(t.extent)+"]":"");}
    const char *names[] = {
        "void",   "bool",        "char",           "int",     "long long",       "float",
        "double", "std::string", "string literal", "ostream", "endl manipulator"};
    return std::string(t.constant ? "const " : "") + names[static_cast<int>(t.base)] +
           (t.reference ? "&" : "") + (t.extent ? "[" + std::to_string(t.extent) + "]" : "");
}
} // namespace praxis
