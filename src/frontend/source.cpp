#include "praxis/common/source.hpp"
#include <fstream>
#include <sstream>
namespace praxis {
void fail(const Location &l, const std::string &p, const std::string &m) {
    throw Error(l, p, m);
}
std::string diagnostic(const Error &e) {
    return e.location.file + ":" + std::to_string(e.location.line) + ":" +
           std::to_string(e.location.column) + ": " + e.phase + ": " + e.what();
}
Source SourceManager::read(const std::filesystem::path &p) const {
    std::ifstream f(p, std::ios::binary);
    if (!f)
        fail({p.string()}, "source", "cannot open file");
    std::ostringstream s;
    s << f.rdbuf();
    return {p.string(), s.str()};
}
} // namespace praxis
