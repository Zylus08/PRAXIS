#pragma once
#include "praxis/frontend/lexer.hpp"
#include <map>
#include <set>
namespace praxis {
class Preprocessor {
    std::map<std::string, std::vector<Token>> macros;
    std::set<std::string> active;
    std::vector<Token> process(const Source &, int);
    std::vector<Token> expand(const std::vector<Token> &, std::set<std::string>, int);

  public:
    std::vector<std::filesystem::path> paths;
    bool iostream = false, string = false;
    std::vector<Token> run(const Source &);
};
} // namespace praxis
