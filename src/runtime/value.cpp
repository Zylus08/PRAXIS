#include "praxis/runtime/value.hpp"
#include <cmath>
#include <limits>
namespace praxis {
namespace {
constexpr auto lo = std::numeric_limits<std::int64_t>::min();
constexpr auto hi = std::numeric_limits<std::int64_t>::max();
std::int64_t integer(const Value &v) {
    return std::get<std::int64_t>(v.data);
}
double real(const Value &v) {
    return std::holds_alternative<double>(v.data) ? std::get<double>(v.data)
                                                  : static_cast<double>(integer(v));
}
std::pair<std::int64_t, std::int64_t> bounds(Base b) {
    if (b == Base::Char)
        return {-128, 127};
    if (b == Base::Int)
        return {-2147483647LL - 1, 2147483647};
    return {lo, hi};
}
} // namespace
Value zero(Type t) {
    t.reference = false;
    t.extent = 0;
    if (t.base == Base::String || t.base == Base::Text)
        return {t, std::string{}};
    if (t.base == Base::Float || t.base == Base::Double)
        return {t, 0.0};
    return {t, std::int64_t{0}};
}
Value literal(const Expr &e) {
    auto t = e.type;
    if (t.base == Base::Text)
        return {t, e.token.text};
    if (t.base == Base::Bool)
        return {t, std::int64_t{e.token.text == "true" ? 1 : 0}};
    if (t.base == Base::Char)
        return {t, static_cast<std::int64_t>(static_cast<signed char>(e.token.text[0]))};
    if (t.base == Base::Float || t.base == Base::Double)
        return convertValue({{Base::Double}, std::stod(e.token.text)}, t, e.token.loc);
    return {t, std::stoll(e.token.text)};
}
bool truth(const Value &v) {
    return std::holds_alternative<double>(v.data) ? std::get<double>(v.data) != 0 : integer(v) != 0;
}
Value read(const Evaluation &e, const Location &l) {
    if (!e.object)
        return e.value;
    if (!e.object->alive)
        fail(l, "runtime", "object lifetime has ended");
    if (!e.object->initialized)
        fail(l, "runtime", "read of uninitialized object");
    if (e.object->type.extent)
        fail(l, "runtime", "array cannot be read as scalar");
    return e.object->value;
}
Value convertValue(Value v, Type t, const Location &l) {
    t.reference = false;
    t.extent = 0;
    if (t.base == Base::String) {
        if (v.type.base == Base::Text) {
            auto &text = std::get<std::string>(v.data);
            text = text.substr(0, text.find(char(0)));
        }
        v.type = t;
        return v;
    }
    if (t.base == Base::Bool)
        return {t, std::int64_t{truth(v) ? 1 : 0}};
    if (t.base == Base::Float || t.base == Base::Double) {
        double d = real(v);
        if (t.base == Base::Float)
            d = std::holds_alternative<std::int64_t>(v.data) ? static_cast<float>(integer(v))
                                                             : static_cast<float>(d);
        if (!std::isfinite(d))
            fail(l, "runtime", "non-finite floating result");
        return {t, d};
    }
    if (integral(t)) {
        auto [low, high] = bounds(t.base);
        std::int64_t n;
        if (std::holds_alternative<double>(v.data)) {
            auto d = std::trunc(std::get<double>(v.data));
            if (!std::isfinite(d) || d < static_cast<double>(low) ||
                (t.base == Base::Long ? d >= 9223372036854775808.0 : d > static_cast<double>(high)))
                fail(l, "runtime", "numeric conversion out of range");
            n = static_cast<std::int64_t>(d);
        } else
            n = integer(v);
        if (n < low || n > high)
            fail(l, "runtime", "numeric conversion out of range");
        return {t, n};
    }
    v.type = t;
    return v;
}
Value binary(const std::string &op, Value a, Value b, const Location &l) {
    auto boolean = [](bool x) { return Value{{Base::Bool}, std::int64_t{x ? 1 : 0}}; };
    if (a.type.base == Base::String || a.type.base == Base::Text || b.type.base == Base::String ||
        b.type.base == Base::Text) {
        auto text = [](const Value &v) {
            return v.type.base == Base::Char
                       ? std::string(1, static_cast<char>(integer(v)))
                       : (v.type.base == Base::Text
                              ? std::get<std::string>(v.data).substr(
                                    0, std::get<std::string>(v.data).find(char(0)))
                              : std::get<std::string>(v.data));
        };
        auto x = text(a), y = text(b);
        if (op == "+")
            return {{Base::String}, x + y};
        if (op == "==")
            return boolean(x == y);
        if (op == "!=")
            return boolean(x != y);
        if (op == "<")
            return boolean(x < y);
        if (op == ">")
            return boolean(x > y);
        if (op == "<=")
            return boolean(x <= y);
        return boolean(x >= y);
    }
    auto t = common(a.type, b.type);
    a = convertValue(a, t, l);
    b = convertValue(b, t, l);
    if (integral(t)) {
        auto x = integer(a), y = integer(b);
        if (op == "==")
            return boolean(x == y);
        if (op == "!=")
            return boolean(x != y);
        if (op == "<")
            return boolean(x < y);
        if (op == ">")
            return boolean(x > y);
        if (op == "<=")
            return boolean(x <= y);
        if (op == ">=")
            return boolean(x >= y);
        std::int64_t z = 0;
        auto overflow = [&]() { fail(l, "runtime", "signed integer overflow"); };
        if (op == "+") {
            if ((y > 0 && x > hi - y) || (y < 0 && x < lo - y))
                overflow();
            z = x + y;
        } else if (op == "-") {
            if ((y < 0 && x > hi + y) || (y > 0 && x < lo + y))
                overflow();
            z = x - y;
        } else if (op == "*") {
            if (x > 0 ? (y > 0 ? x > hi / y : y < lo / x)
                      : (x < 0 ? (y > 0 ? x < lo / y : (y < 0 && x < hi / y)) : false))
                overflow();
            z = x * y;
        } else if (op == "/" || op == "%") {
            if (!y)
                fail(l, "runtime", "division by zero");
            if (x == lo && y == -1)
                overflow();
            z = op == "/" ? x / y : x % y;
        } else
            fail(l, "runtime", "unsupported arithmetic operator");
        auto [low, high] = bounds(t.base);
        if (z < low || z > high)
            overflow();
        return {t, z};
    }
    auto x = real(a), y = real(b);
    if (op == "==")
        return boolean(x == y);
    if (op == "!=")
        return boolean(x != y);
    if (op == "<")
        return boolean(x < y);
    if (op == ">")
        return boolean(x > y);
    if (op == "<=")
        return boolean(x <= y);
    if (op == ">=")
        return boolean(x >= y);
    double z = 0;
    if (op == "+")
        z = x + y;
    else if (op == "-")
        z = x - y;
    else if (op == "*")
        z = x * y;
    else if (op == "/") {
        if (y == 0)
            fail(l, "runtime", "division by zero");
        z = x / y;
    } else
        fail(l, "runtime", "unsupported floating operator");
    return convertValue({t, z}, t, l);
}
} // namespace praxis
