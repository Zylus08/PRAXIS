#pragma once
#include "praxis/runtime/value.hpp"
#include <ostream>
namespace praxis {
void output(std::ostream &, const Value &);
Value stringMember(const std::string &, const Value &);
} // namespace praxis
