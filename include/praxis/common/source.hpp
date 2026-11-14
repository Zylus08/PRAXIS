#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
namespace praxis {
struct Location {
    std::string file;
    int line = 1, column = 1;
};
class Error : public std::runtime_error {
  public:
    Location location;
    std::string phase;
    Error(Location l, std::string p, std::string m)
        : std::runtime_error(std::move(m)), location(std::move(l)), phase(std::move(p)) {}
};
[[noreturn]] void fail(const Location &, const std::string &, const std::string &);
std::string diagnostic(const Error &);
struct Source {
    std::string name, text;
};
class SourceManager {
  public:
    Source read(const std::filesystem::path &) const;
};
} // namespace praxis
