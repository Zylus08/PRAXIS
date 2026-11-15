#include "praxis/native/bindings.hpp"
namespace praxis {
void output(std::ostream &out, const Value &v) {
    if (std::holds_alternative<Pointer>(v.data)) {
        const auto &p = std::get<Pointer>(v.data);
        if (p.isNull || p.targetId == 0)
            out << "0";
        else
            out << p.targetId;
    } else if (std::holds_alternative<std::string>(v.data))
        out << (v.type.base == Base::Text ? std::get<std::string>(v.data).substr(
                                                0, std::get<std::string>(v.data).find(char(0)))
                                          : std::get<std::string>(v.data));
    else if (std::holds_alternative<double>(v.data))
        out << std::get<double>(v.data);
    else if (v.type.base == Base::Char)
        out << static_cast<char>(std::get<std::int64_t>(v.data));
    else
        out << std::get<std::int64_t>(v.data);
}
Value stringMember(const std::string &name, const Value &v) {
    const auto &s = std::get<std::string>(v.data);
    if (name == "empty")
        return {{Base::Bool}, std::int64_t{s.empty() ? 1 : 0}};
    return {{Base::Long}, static_cast<std::int64_t>(s.size())};
}
} // namespace praxis
