#include "praxis/frontend/preprocessor.hpp"
#include <algorithm>
namespace praxis {
std::vector<Token> Preprocessor::expand(const std::vector<Token> &ts,
                                        std::set<std::string> disabled, int depth) {
    if (depth > 100)
        fail(ts.empty() ? Location{} : ts[0].loc, "preprocess", "macro expansion limit");
    std::vector<Token> out;
    for (auto t : ts) {
        auto it = macros.find(t.text);
        if (t.kind == TokenKind::Identifier && it != macros.end() && !disabled.contains(t.text)) {
            auto d = disabled;
            d.insert(t.text);
            auto replacement = it->second;
            for (auto &r : replacement)
                r.loc = t.loc;
            auto e = expand(replacement, d, depth + 1);
            out.insert(out.end(), e.begin(), e.end());
        } else
            out.push_back(t);
        if (out.size() > 1000000)
            fail(t.loc, "preprocess", "token limit");
    }
    return out;
}
std::vector<Token> Preprocessor::process(const Source &s, int depth) {
    if (depth > 64)
        fail({s.name}, "preprocess", "include depth limit");
    auto key = std::filesystem::absolute(s.name).lexically_normal().string();
    if (!active.insert(key).second)
        fail({s.name}, "preprocess", "include cycle");
    struct Exit {
        std::set<std::string> &a;
        std::string k;
        ~Exit() {
            a.erase(k);
        }
    } exit{active, key};
    auto ts = lex(s);
    std::vector<Token> out;
    for (size_t i = 0; i + 1 < ts.size();) {
        size_t end = i + 1;
        while (end + 1 < ts.size() && ts[end].loc.line == ts[i].loc.line)
            ++end;
        std::vector<Token> line(ts.begin() + static_cast<ptrdiff_t>(i),
                                ts.begin() + static_cast<ptrdiff_t>(end));
        i = end;
        if (!line[0].is("#")) {
            auto e = expand(line, {}, 0);
            for (auto &t : e) {
                t.iostreamAvailable = iostream;
                t.stringAvailable = string;
            }
            out.insert(out.end(), e.begin(), e.end());
            continue;
        }
        auto loc = line[0].loc;
        if (line.size() < 2)
            fail(loc, "preprocess", "missing directive");
        if (line[1].kind != TokenKind::Identifier && line[1].kind != TokenKind::Keyword)
            fail(loc, "preprocess", "expected directive name");
        auto op = line[1].text;
        if (op == "define") {
            if (line.size() < 3 || line[2].kind != TokenKind::Identifier)
                fail(loc, "preprocess", "expected macro name");
            if (line.size() > 3 && line[3].text == "(" &&
                line[3].loc.column == line[2].loc.column + static_cast<int>(line[2].text.size()))
                fail(loc, "preprocess", "function-like macros are unsupported");
            auto name = line[2].text;
            if (macros.contains(name))
                fail(loc, "preprocess", "macro redefinition requires undef");
            macros[name] = {line.begin() + 3, line.end()};
        } else if (op == "undef") {
            if (line.size() != 3 || line[2].kind != TokenKind::Identifier)
                fail(loc, "preprocess", "expected macro name");
            macros.erase(line[2].text);
        } else if (op == "include") {
            std::string name;
            bool angle = false;
            if (line.size() == 3 && line[2].kind == TokenKind::String)
                name = line[2].text;
            else if (line.size() >= 5 && line[2].text == "<" && line.back().text == ">") {
                angle = true;
                for (size_t j = 3; j + 1 < line.size(); ++j)
                    name += line[j].text;
            } else
                fail(loc, "preprocess", "expected quoted or bracketed header");
            if (angle && (name == "iostream" || name == "string")) {
                if (name == "iostream")
                    iostream = true;
                else
                    string = true;
                continue;
            }
            std::vector<std::filesystem::path> search = paths;
            if (!angle)
                search.insert(search.begin(), std::filesystem::path(s.name).parent_path());
            bool found = false;
            for (const auto &dir : search) {
                auto p = dir / name;
                if (std::filesystem::is_regular_file(p)) {
                    auto e = process(SourceManager{}.read(p), depth + 1);
                    out.insert(out.end(), e.begin(), e.end());
                    found = true;
                    break;
                }
            }
            if (!found)
                fail(loc, "preprocess", "unsupported or missing header: " + name);
        } else
            fail(loc, "preprocess", "unsupported directive: " + op);
    }
    return out;
}
std::vector<Token> Preprocessor::run(const Source &s) {
    auto t = process(s, 0);
    t.push_back(
        {TokenKind::End,
         "",
         {s.name, static_cast<int>(1 + std::count(s.text.begin(), s.text.end(), '\n')), 1}});
    return t;
}
} // namespace praxis
