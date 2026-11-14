#include "praxis/frontend/parser.hpp"
#include "praxis/frontend/preprocessor.hpp"
#include "praxis/runtime/interpreter.hpp"
#include "praxis/semantics/analyzer.hpp"
#include <iostream>
namespace praxis {
Program prepare(const Source &s, Preprocessor &pp, bool script = false) {
    auto p = Parser(pp.run(s)).parse(script);
    p.iostream = pp.iostream;
    p.strings = pp.string;
    return p;
}
int repl() {
    Program history;
    history.script = true;
    Preprocessor pp;
    auto runtime = std::make_unique<Interpreter>(std::cout);
    std::string line, buffer;
    int submission = 0;
    std::cout << "Praxis M1 REPL (:quit, :reset; finish multiline input with an empty line)\n";
    while (std::getline(std::cin, line)) {
        if (buffer.empty() && line == ":quit")
            break;
        if (buffer.empty() && line == ":reset") {
            history = Program{};
            history.script = true;
            pp = Preprocessor{};
            runtime = std::make_unique<Interpreter>(std::cout);
            std::cout << "State reset.\n";
            continue;
        }
        buffer += line + '\n';
        int braces = 0;
        bool quoted = false, escaped = false;
        for (char c : buffer) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (c == '\\') {
                escaped = true;
                continue;
            }
            if (c == '"') {
                quoted = !quoted;
                continue;
            }
            if (!quoted) {
                if (c == '{')
                    ++braces;
                if (c == '}')
                    --braces;
            }
        }
        if (!line.empty() && (braces > 0 || quoted))
            continue;
        if (buffer == "\n") {
            buffer.clear();
            continue;
        }
        try {
            auto candidatePP = pp;
            auto chunk =
                prepare({"<repl:" + std::to_string(++submission) + ">", buffer}, candidatePP, true);
            auto candidate = history;
            size_t start = candidate.items.size();
            candidate.items.insert(candidate.items.end(), chunk.items.begin(), chunk.items.end());
            candidate.iostream = candidatePP.iostream;
            candidate.strings = candidatePP.string;
            Analyzer{}.check(candidate);
            history = std::move(candidate);
            pp = std::move(candidatePP);
            runtime->run(history, start);
        } catch (const Error &e) {
            std::cerr << diagnostic(e) << '\n';
        }
        buffer.clear();
    }
    if (!buffer.empty()) {
        std::cerr << "<repl>:1:1: parse: incomplete submission\n";
        return 1;
    }
    return 0;
}
} // namespace praxis
int main(int argc, char **argv) {
    using namespace praxis;
    try {
        if (argc == 2 && std::string(argv[1]) == "--version") {
            std::cout << "Praxis 0.1.0 (M1 subset)\n";
            return 0;
        }
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) {
            std::cout << "Usage: praxis --version | --help | repl | run FILE [-I DIR] | check FILE "
                         "[-I DIR]\n";
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "repl")
            return repl();
        if (argc < 3 || (std::string(argv[1]) != "run" && std::string(argv[1]) != "check")) {
            std::cerr << "praxis: invalid command; use --help\n";
            return 2;
        }
        Preprocessor pp;
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "-I" && i + 1 < argc)
                pp.paths.emplace_back(argv[++i]);
            else if (a.starts_with("-I") && a.size() > 2)
                pp.paths.emplace_back(a.substr(2));
            else {
                std::cerr << "praxis: invalid option: " << a << '\n';
                return 2;
            }
        }
        auto p = prepare(SourceManager{}.read(argv[2]), pp);
        Analyzer{}.check(p);
        if (std::string(argv[1]) == "check")
            return 0;
        return Interpreter(std::cout).run(p);
    } catch (const Error &e) {
        std::cerr << diagnostic(e) << '\n';
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "praxis: internal/resource error: " << e.what() << '\n';
        return 2;
    }
}
