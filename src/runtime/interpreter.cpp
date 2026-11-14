#include "praxis/runtime/interpreter.hpp"
#include "praxis/native/bindings.hpp"
#include <functional>
#include <set>
namespace praxis {
namespace {
void expire(const std::shared_ptr<Object> &o) {
    o->alive = false;
    for (auto e : o->elements)
        expire(e);
}
} // namespace
Scope::~Scope() {
    for (auto o : owned)
        expire(o);
}
Scope &Interpreter::current() {
    return frames.empty() ? globals : *frames.back()->scopes.back();
}
void Interpreter::tick(const Location &l) {
    if (++steps > 10000000)
        fail(l, "runtime", "execution step limit exceeded");
}
std::shared_ptr<Object> Interpreter::lookup(const Token &t) {
    if (!frames.empty())
        for (auto it = frames.back()->scopes.rbegin(); it != frames.back()->scopes.rend(); ++it) {
            auto f = (*it)->names.find(t.text);
            if (f != (*it)->names.end())
                return f->second;
        }
    auto f = globals.names.find(t.text);
    if (f != globals.names.end())
        return f->second;
    fail(t.loc, "runtime", "object not yet initialized: " + t.text);
}
std::shared_ptr<Object> Interpreter::allocate(Type t, bool initialized) {
    auto o = std::make_shared<Object>();
    o->type = t;
    o->type.reference = false;
    o->value = zero(t);
    o->initialized = initialized || t.base == Base::String;
    if (t.extent) {
        Type element = t;
        element.extent = 0;
        for (int i = 0; i < t.extent; ++i)
            o->elements.push_back(allocate(element, initialized));
        o->initialized = true;
    }
    return o;
}
Value Interpreter::value(const E &e) {
    return read(eval(e), e->token.loc);
}
Evaluation Interpreter::eval(const E &e) {
    if (evalDepth >= 256)
        fail(e->token.loc, "runtime", "evaluation depth limit exceeded");
    ++evalDepth;
    struct DepthExit {
        int &d;
        ~DepthExit() {
            --d;
        }
    } depthExit{evalDepth};
    tick(e->token.loc);
    auto l = e->token.loc;
    switch (e->kind) {
    case Expr::Kind::Literal:
        return {literal(*e), {}};
    case Expr::Kind::Name:
        if (e->op == "std::cout")
            return {{{Base::Stream}, std::int64_t{0}}, {}};
        if (e->op == "std::endl")
            return {{{Base::Endl}, std::string("\n")}, {}};
        return {{}, lookup(e->token)};
    case Expr::Kind::Index: {
        auto a = eval(e->args[0]).object;
        auto i = std::get<std::int64_t>(value(e->args[1]).data);
        if (!a || !a->alive)
            fail(l, "runtime", "invalid array lifetime");
        if (i < 0 || i >= static_cast<std::int64_t>(a->elements.size()))
            fail(l, "runtime", "array index out of bounds");
        return {{}, a->elements[static_cast<size_t>(i)]};
    }
    case Expr::Kind::Member:
        return {stringMember(e->op, value(e->args[0])), {}};
    case Expr::Kind::Call: {
        std::vector<Evaluation> args;
        auto f = functions.at(e->args[0]->op);
        for (size_t i = 1; i < e->args.size(); ++i) {
            auto a = eval(e->args[i]);
            if (!f->params[i - 1].type.reference) {
                a.value = read(a, l);
                a.object.reset();
            }
            args.push_back(a);
        }
        return {call(f, args), {}};
    }
    case Expr::Kind::Unary:
    case Expr::Kind::Postfix: {
        auto a = eval(e->args[0]);
        auto v = read(a, l);
        if (e->op == "!")
            return {{{Base::Bool}, std::int64_t{truth(v) ? 0 : 1}}, {}};
        if (e->op == "++" || e->op == "--") {
            auto next = binary(e->op == "++" ? "+" : "-", v, {{Base::Int}, std::int64_t{1}}, l);
            a.object->value = convertValue(next, a.object->type, l);
            if (e->kind == Expr::Kind::Postfix)
                return {v, {}};
            return a;
        }
        if (e->op == "+")
            return {convertValue(v, e->type, l), {}};
        if (e->type.base == Base::Float || e->type.base == Base::Double)
            return {convertValue({e->type, -std::get<double>(v.data)}, e->type, l), {}};
        return {binary("-", zero(e->type), v, l), {}};
    }
    case Expr::Kind::Assign: {
        auto b = value(e->args[1]);
        auto a = eval(e->args[0]);
        if (!a.object || !a.object->alive)
            fail(l, "runtime", "invalid assignment target");
        if (e->op != "=")
            b = binary(e->op.substr(0, 1), read(a, l), b, l);
        a.object->value = convertValue(b, a.object->type, l);
        a.object->initialized = true;
        return a;
    }
    case Expr::Kind::Binary: {
        auto a = value(e->args[0]);
        if (e->op == "&&" && !truth(a))
            return {{{Base::Bool}, std::int64_t{0}}, {}};
        if (e->op == "||" && truth(a))
            return {{{Base::Bool}, std::int64_t{1}}, {}};
        auto b = value(e->args[1]);
        if (e->op == "&&" || e->op == "||")
            return {{{Base::Bool}, std::int64_t{truth(b) ? 1 : 0}}, {}};
        if (e->op == "<<" && a.type.base == Base::Stream) {
            output(out, b);
            if (e->args[1]->kind == Expr::Kind::Name && e->args[1]->op == "std::endl")
                out.flush();
            return {a, {}};
        }
        return {binary(e->op, a, b, l), {}};
    }
    }
    fail(l, "runtime", "unknown expression");
}
void Interpreter::declare(const Decl &d) {
    auto &scope = current();
    if (d.type.reference) {
        auto a = eval(d.init);
        scope.names[d.name.text] = a.object;
        return;
    }
    auto prior = scope.names.find(d.name.text);
    auto o = frames.empty() && prior != scope.names.end() ? prior->second
                                                          : allocate(d.type, frames.empty());
    if (prior == scope.names.end()) {
        scope.owned.push_back(o);
        scope.names[d.name.text] = o;
    }
    if (d.type.extent) {
        if (d.braced) {
            for (size_t i = 0; i < o->elements.size(); ++i) {
                auto cell = o->elements[i];
                if (i < d.elements.size())
                    cell->value = convertValue(value(d.elements[i]), cell->type, d.name.loc);
                cell->initialized = true;
            }
        }
    } else if (d.init) {
        o->value = convertValue(value(d.init), d.type, d.name.loc);
        o->initialized = true;
    }
}
Flow Interpreter::nested(const S &s) {
    frames.back()->scopes.push_back(std::make_unique<Scope>());
    struct Exit {
        Frame &f;
        ~Exit() {
            f.scopes.pop_back();
        }
    } exit{*frames.back()};
    return execute(s, false);
}
Flow Interpreter::execute(const S &s, bool scoped) {
    tick(s->token.loc);
    switch (s->kind) {
    case Stmt::Kind::Block: {
        if (scoped)
            return nested(s);
        for (auto c : s->children) {
            auto f = execute(c);
            if (f.kind != Flow::Normal)
                return f;
        }
        break;
    }
    case Stmt::Kind::Declaration:
        declare(s->decl);
        break;
    case Stmt::Kind::Expression:
        eval(s->expr);
        break;
    case Stmt::Kind::If:
        if (truth(value(s->condition)))
            return nested(s->children[0]);
        else if (s->children.size() > 1)
            return nested(s->children[1]);
        break;
    case Stmt::Kind::While:
    case Stmt::Kind::Do: {
        bool first = s->kind == Stmt::Kind::Do;
        while (first || truth(value(s->condition))) {
            first = false;
            auto f = nested(s->children[0]);
            if (f.kind == Flow::Return)
                return f;
            if (f.kind == Flow::Break)
                break;
        }
        break;
    }
    case Stmt::Kind::For: {
        frames.back()->scopes.push_back(std::make_unique<Scope>());
        struct Exit {
            Frame &f;
            ~Exit() {
                f.scopes.pop_back();
            }
        } exit{*frames.back()};
        execute(s->children[0]);
        while (!s->condition || truth(value(s->condition))) {
            auto f = nested(s->children[1]);
            if (f.kind == Flow::Return)
                return f;
            if (f.kind == Flow::Break)
                break;
            if (s->step)
                eval(s->step);
            tick(s->token.loc);
        }
        break;
    }
    case Stmt::Kind::Return:
        return {Flow::Return, s->expr ? value(s->expr) : zero({Base::Void})};
    case Stmt::Kind::Break:
        return {Flow::Break, {}};
    case Stmt::Kind::Continue:
        return {Flow::Continue, {}};
    case Stmt::Kind::Empty:
        break;
    }
    return {};
}
Value Interpreter::call(const std::shared_ptr<Function> &f, const std::vector<Evaluation> &args) {
    if (frames.size() >= 128)
        fail(f->name.loc, "runtime", "call depth limit exceeded");
    auto frame = std::make_unique<Frame>();
    frame->scopes.push_back(std::make_unique<Scope>());
    frames.push_back(std::move(frame));
    struct Exit {
        std::vector<std::unique_ptr<Frame>> &f;
        ~Exit() {
            f.pop_back();
        }
    } exit{frames};
    for (size_t i = 0; i < args.size(); ++i) {
        auto &d = f->params[i];
        if (d.type.reference)
            current().names[d.name.text] = args[i].object;
        else {
            auto o = allocate(d.type, true);
            o->value = convertValue(read(args[i], d.name.loc), d.type, d.name.loc);
            current().owned.push_back(o);
            current().names[d.name.text] = o;
        }
    }
    auto flow = execute(f->body, false);
    if (flow.kind == Flow::Return)
        return f->result.base == Base::Void ? zero(f->result)
                                            : convertValue(flow.value, f->result, f->name.loc);
    return zero(f->result);
}
int Interpreter::run(const Program &p, size_t start) {
    resetBudget();
    for (auto &i : p.items)
        if (i.function && i.function->body)
            functions[i.function->name.text] = i.function;
    for (size_t i = start; i < p.items.size(); ++i) {
        auto s = p.items[i].statement;
        if (s && s->kind == Stmt::Kind::Declaration && !s->decl.type.reference) {
            auto o = allocate(s->decl.type, true);
            globals.names[s->decl.name.text] = o;
            globals.owned.push_back(o);
        }
    }
    // Constant initialization precedes all dynamic global initializers.
    std::set<std::string> constantObjects, initializedEarly;
    std::function<bool(const E &)> constantExpression = [&](const E &e) {
        if (e->kind == Expr::Kind::Literal)
            return numeric(e->type);
        if (e->kind == Expr::Kind::Name)
            return constantObjects.contains(e->op);
        if (e->kind == Expr::Kind::Unary)
            return (e->op == "+" || e->op == "-" || e->op == "!") && constantExpression(e->args[0]);
        if (e->kind != Expr::Kind::Binary || !numeric(e->type) || !constantExpression(e->args[0]))
            return false;
        if (e->op == "&&" && !truth(value(e->args[0])))
            return true;
        if (e->op == "||" && truth(value(e->args[0])))
            return true;
        return constantExpression(e->args[1]);
    };
    for (size_t i = start; i < p.items.size(); ++i) {
        auto s = p.items[i].statement;
        if (!s || s->kind != Stmt::Kind::Declaration)
            continue;
        auto &d = s->decl;
        bool early = false;
        if (d.type.reference)
            early =
                d.init && d.init->kind == Expr::Kind::Name && globals.names.contains(d.init->op);
        else if (numeric(d.type) && d.init)
            early = constantExpression(d.init);
        else if (d.type.extent && d.braced && d.type.base != Base::String) {
            early = true;
            for (auto e : d.elements)
                early = early && constantExpression(e);
        }
        if (early) {
            declare(d);
            initializedEarly.insert(d.name.text);
            if (d.type.constant && !d.type.reference && integral(d.type))
                constantObjects.insert(d.name.text);
        }
    }
    // Script top-level declarations have static storage; temporary statement frames resolve them
    // globally.
    for (size_t i = start; i < p.items.size(); ++i)
        if (p.items[i].statement) {
            auto s = p.items[i].statement;
            if (s->kind == Stmt::Kind::Declaration) {
                if (!initializedEarly.contains(s->decl.name.text))
                    declare(s->decl);
            } else {
                auto f = std::make_unique<Frame>();
                f->scopes.push_back(std::make_unique<Scope>());
                frames.push_back(std::move(f));
                try {
                    execute(s);
                    frames.pop_back();
                } catch (...) {
                    frames.pop_back();
                    throw;
                }
            }
        }
    if (p.script)
        return 0;
    return static_cast<int>(std::get<std::int64_t>(call(functions.at("main"), {}).data));
}
} // namespace praxis
