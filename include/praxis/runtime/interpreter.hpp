#pragma once
#include "praxis/runtime/value.hpp"
#include <map>
#include <ostream>
namespace praxis {
struct Scope {
    std::map<std::string, std::shared_ptr<Object>> names;
    std::vector<std::shared_ptr<Object>> owned;
    ~Scope();
};
struct Frame {
    std::vector<std::unique_ptr<Scope>> scopes;
};
struct Flow {
    enum Kind { Normal, Return, Break, Continue } kind = Normal;
    Value value;
};
class Interpreter {
    Scope globals;
    std::vector<std::unique_ptr<Frame>> frames;
    std::map<std::string, std::shared_ptr<Function>> functions;
    std::ostream &out;
    std::uint64_t steps = 0;
    int evalDepth = 0;
    Scope &current();
    std::shared_ptr<Object> lookup(const Token &);
    std::shared_ptr<Object> allocate(Type, bool);
    Evaluation eval(const E &);
    Value value(const E &);
    Flow execute(const S &, bool = true);
    Flow nested(const S &);
    void declare(const Decl &);
    Value call(const std::shared_ptr<Function> &, const std::vector<Evaluation> &);
    void tick(const Location &);

  public:
    explicit Interpreter(std::ostream &o) : out(o) {}
    int run(const Program &, size_t start = 0);
    void resetBudget() {
        steps = 0;
    }
};
} // namespace praxis
