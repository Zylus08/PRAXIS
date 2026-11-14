#include "praxis/semantics/analyzer.hpp"
#include <cmath>
#include <limits>
namespace praxis {
void Analyzer::bind(const Token &n, Type t) {
    if (n.text == "std")
        fail(n.loc, "semantic", "std is reserved for native bindings");
    if (scopes.back().contains(n.text) || (scopes.size() == 1 && functions.contains(n.text)))
        fail(n.loc, "semantic", "duplicate declaration: " + n.text);
    scopes.back()[n.text] = t;
}
Type Analyzer::lookup(const Token &n) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto x = it->find(n.text);
        if (x != it->end())
            return x->second;
    }
    fail(n.loc, "semantic", "unknown name: " + n.text);
}
void Analyzer::modifiable(const E &e) {
    if (!e->lvalue || e->type.constant || e->type.extent)
        fail(e->token.loc, "semantic", "expression is not a modifiable scalar lvalue");
}
void Analyzer::convert(const E &e, Type t) {
    auto s = e->type;
    if (t.reference) {
        if (!e->lvalue || s.extent || s.base != t.base || (!t.constant && s.constant))
            fail(e->token.loc, "semantic", "reference requires compatible lvalue");
        return;
    }
    if (s.extent || t.extent || s.base == Base::Void || t.base == Base::Void)
        fail(e->token.loc, "semantic", "invalid value conversion");
    if (numeric(s) && numeric(t))
        return;
    if (t.base == Base::String && (s.base == Base::String || s.base == Base::Text))
        return;
    if (t.base == s.base && t.base != Base::Stream)
        return;
    fail(e->token.loc, "semantic", "cannot convert " + typeName(s) + " to " + typeName(t));
}
void Analyzer::condition(const E &e) {
    if (e && !numeric(expr(e)))
        fail(e->token.loc, "semantic", "condition requires numeric or bool value");
}
Type Analyzer::expr(const E &e) {
    auto error = [&](const std::string &m) { fail(e->token.loc, "semantic", m); };
    e->lvalue = false;
    Type t;
    switch (e->kind) {
    case Expr::Kind::Literal: {
        auto &tok = e->token;
        if (tok.kind == TokenKind::Integer) {
            try {
                size_t used = 0;
                auto n = std::stoll(tok.text, &used);
                bool suffix = used < tok.text.size();
                t.base = suffix || n > std::numeric_limits<int>::max() ? Base::Long : Base::Int;
            } catch (...) {
                error("integer literal out of range");
            }
        } else if (tok.kind == TokenKind::Floating) {
            try {
                auto n = std::stod(tok.text);
                bool single = tok.text.back() == 'f' || tok.text.back() == 'F';
                if (!std::isfinite(n) || (single && !std::isfinite(static_cast<float>(n))))
                    error("floating literal out of range");
                t.base = single ? Base::Float : Base::Double;
            } catch (const Error &) {
                throw;
            } catch (...) {
                error("floating literal out of range");
            }
        } else if (tok.kind == TokenKind::Character)
            t.base = Base::Char;
        else if (tok.kind == TokenKind::String) {
            t.base = Base::Text;
            t.constant = true;
        } else
            t.base = Base::Bool;
        break;
    }
    case Expr::Kind::Name:
        if (e->op == "std::cout") {
            if (!e->token.iostreamAvailable)
                error("std::cout requires <iostream>");
            t.base = Base::Stream;
        } else if (e->op == "std::endl") {
            if (!e->token.iostreamAvailable)
                error("std::endl requires <iostream>");
            t.base = Base::Endl;
            t.constant = true;
        } else {
            t = lookup(e->token);
            t.reference = false;
            e->lvalue = true;
        }
        break;
    case Expr::Kind::Index: {
        auto a = expr(e->args[0]);
        auto b = expr(e->args[1]);
        if (!a.extent || !integral(b))
            error("index requires fixed array and integral index");
        t = a;
        t.extent = 0;
        e->lvalue = true;
        break;
    }
    case Expr::Kind::Member: {
        auto a = expr(e->args[0]);
        if (a.base != Base::String || a.extent)
            error("member operation requires std::string");
        if (e->op == "size" || e->op == "length")
            t.base = Base::Long;
        else if (e->op == "empty")
            t.base = Base::Bool;
        else
            error("unsupported string member");
        break;
    }
    case Expr::Kind::Call: {
        auto callee = e->args[0];
        if (callee->kind != Expr::Kind::Name)
            error("only named functions can be called");
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            if (it->contains(callee->op))
                error("called name denotes an object");
        auto it = functions.find(callee->op);
        if (it == functions.end())
            error("function must be declared before use: " + callee->op);
        auto f = it->second;
        if (f->name.text == "main")
            error("calling main is unsupported");
        if (e->args.size() != f->params.size() + 1)
            error("argument count mismatch");
        for (size_t i = 0; i < f->params.size(); ++i) {
            expr(e->args[i + 1]);
            convert(e->args[i + 1], f->params[i].type);
        }
        called.insert(callee->op);
        t = f->result;
        break;
    }
    case Expr::Kind::Unary:
    case Expr::Kind::Postfix: {
        auto a = expr(e->args[0]);
        if (!numeric(a))
            error("unary operator requires arithmetic value");
        if (e->op == "++" || e->op == "--") {
            modifiable(e->args[0]);
            if (a.base == Base::Bool)
                error("bool increment/decrement is invalid");
            t = a;
            e->lvalue = e->kind == Expr::Kind::Unary;
        } else
            t = e->op == "!" ? Type{Base::Bool} : promoted(a);
        break;
    }
    case Expr::Kind::Assign: {
        auto a = expr(e->args[0]);
        auto b = expr(e->args[1]);
        modifiable(e->args[0]);
        if (e->op != "=") {
            if (a.base == Base::String && e->op == "+=" &&
                (b.base == Base::String || b.base == Base::Text || b.base == Base::Char)) {
            } else if (!numeric(a) || !numeric(b) ||
                       (e->op == "%=" && (!integral(a) || !integral(b))))
                error("invalid compound assignment");
        } else
            convert(e->args[1], a);
        t = a;
        e->lvalue = true;
        break;
    }
    case Expr::Kind::Binary: {
        auto a = expr(e->args[0]);
        auto b = expr(e->args[1]);
        auto op = e->op;
        if (op == "<<" && a.base == Base::Stream) {
            if (b.extent || b.base == Base::Void || b.base == Base::Stream)
                error("unsupported stream operand");
            t = a;
            break;
        }
        bool textA = a.base == Base::String || a.base == Base::Text,
             textB = b.base == Base::String || b.base == Base::Text;
        bool compare =
            op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=";
        if (!a.extent && !b.extent &&
            ((textA && textB) || (a.base == Base::String && b.base == Base::Char) ||
             (a.base == Base::Char && b.base == Base::String)) &&
            (a.base == Base::String || b.base == Base::String)) {
            if (op == "+")
                t.base = Base::String;
            else if (compare && textA && textB)
                t.base = Base::Bool;
            else
                error("invalid string operator");
            break;
        }
        if (!numeric(a) || !numeric(b))
            error("operator requires arithmetic operands");
        if (op == "<<" || op == ">>")
            error("integer shifts are unsupported");
        if (op == "%" && (!integral(a) || !integral(b)))
            error("remainder requires integral operands");
        t = compare || op == "&&" || op == "||" ? Type{Base::Bool} : common(a, b);
        break;
    }
    }
    e->type = t;
    return t;
}
void Analyzer::declare(Decl &d) {
    auto t = d.type;
    if (t.base == Base::Void || t.base == Base::Stream)
        fail(d.name.loc, "semantic", "invalid object type");
    if (t.base == Base::String && !d.name.stringAvailable)
        fail(d.name.loc, "semantic", "std::string requires <string>");
    if (t.extent && t.reference)
        fail(d.name.loc, "semantic", "arrays of references are invalid");
    bind(d.name, t);
    if (t.extent) {
        if (d.init)
            fail(d.name.loc, "semantic", "array initializer must be braced");
        if (d.elements.size() > static_cast<size_t>(t.extent))
            fail(d.name.loc, "semantic", "too many array initializers");
        t.extent = 0;
        for (auto &e : d.elements) {
            expr(e);
            convert(e, t);
            if (numeric(t) && e->type.base > t.base)
                fail(e->token.loc, "semantic", "narrowing list initialization is unsupported");
        }
        if (t.constant && !d.braced)
            fail(d.name.loc, "semantic", "const array requires initializer");
    } else {
        if (d.braced)
            fail(d.name.loc, "semantic", "scalar brace initialization is unsupported");
        if (d.init) {
            expr(d.init);
            convert(d.init, t);
        } else if (t.reference || t.constant)
            fail(d.name.loc, "semantic", "reference/const object requires initializer");
    }
}
bool Analyzer::returns(const S &s) const {
    if (s->kind == Stmt::Kind::Return)
        return true;
    if (s->kind == Stmt::Kind::Block) {
        for (auto c : s->children)
            if (returns(c))
                return true;
    }
    if (s->kind == Stmt::Kind::If && s->children.size() == 2)
        return returns(s->children[0]) && returns(s->children[1]);
    return false;
}
void Analyzer::stmt(const S &s, bool scoped) {
    auto nested = [&](const S &c) {
        scopes.emplace_back();
        stmt(c, false);
        scopes.pop_back();
    };
    switch (s->kind) {
    case Stmt::Kind::Block:
        if (scoped)
            scopes.emplace_back();
        for (auto c : s->children)
            stmt(c);
        if (scoped)
            scopes.pop_back();
        break;
    case Stmt::Kind::Declaration:
        declare(s->decl);
        break;
    case Stmt::Kind::Expression:
        expr(s->expr);
        break;
    case Stmt::Kind::If:
        condition(s->condition);
        for (auto c : s->children)
            nested(c);
        break;
    case Stmt::Kind::While:
    case Stmt::Kind::Do:
        condition(s->condition);
        ++loops;
        nested(s->children[0]);
        --loops;
        break;
    case Stmt::Kind::For:
        scopes.emplace_back();
        if (s->children[0]->kind != Stmt::Kind::Declaration &&
            s->children[0]->kind != Stmt::Kind::Expression &&
            s->children[0]->kind != Stmt::Kind::Empty)
            fail(s->token.loc, "semantic", "invalid for initializer");
        stmt(s->children[0]);
        condition(s->condition);
        if (s->step)
            expr(s->step);
        ++loops;
        stmt(s->children[1], false);
        --loops;
        scopes.pop_back();
        break;
    case Stmt::Kind::Return:
        if (!inFunction)
            fail(s->token.loc, "semantic", "return outside function");
        if (s->expr) {
            expr(s->expr);
            convert(s->expr, result);
        } else if (result.base != Base::Void)
            fail(s->token.loc, "semantic", "missing return value");
        break;
    case Stmt::Kind::Break:
    case Stmt::Kind::Continue:
        if (!loops)
            fail(s->token.loc, "semantic", "loop control outside loop");
        break;
    case Stmt::Kind::Empty:
        break;
    }
}
void Analyzer::check(Program &p) {
    program = &p;
    scopes = {{}};
    functions.clear();
    definitions.clear();
    called.clear();
    inFunction = false;
    loops = 0;
    for (auto &item : p.items) {
        if (item.statement) {
            stmt(item.statement);
            continue;
        }
        auto f = item.function;
        if (f->name.text == "std")
            fail(f->name.loc, "semantic", "std is reserved for native bindings");
        if (f->result.reference || f->result.constant)
            fail(f->name.loc, "semantic", "qualified/reference return types are unsupported");
        if (f->result.base == Base::String && !f->name.stringAvailable)
            fail(f->name.loc, "semantic", "std::string requires <string>");
        if (scopes[0].contains(f->name.text))
            fail(f->name.loc, "semantic", "function conflicts with object");
        auto old = functions.find(f->name.text);
        if (old != functions.end()) {
            auto g = old->second;
            bool same = g->result == f->result && g->params.size() == f->params.size();
            if (same)
                for (size_t i = 0; i < f->params.size(); ++i) {
                    auto a = g->params[i].type, b = f->params[i].type;
                    if (!a.reference)
                        a.constant = false;
                    if (!b.reference)
                        b.constant = false;
                    same = same && a == b;
                }
            if (!same)
                fail(f->name.loc, "semantic", "conflicting signature; overloading unsupported");
        }
        functions[f->name.text] = f;
        if (f->name.text == "main" && (f->result.base != Base::Int || !f->params.empty()))
            fail(f->name.loc, "semantic", "main must have signature int main()");
        scopes.emplace_back();
        for (auto &d : f->params) {
            if (d.type.base == Base::Void ||
                (d.type.base == Base::String && !d.name.stringAvailable))
                fail(d.name.loc, "semantic", "invalid parameter type");
            bind(d.name, d.type);
        }
        if (f->body) {
            if (!definitions.insert(f->name.text).second)
                fail(f->name.loc, "semantic", "duplicate function definition");
            inFunction = true;
            result = f->result;
            stmt(f->body, false);
            if (result.base != Base::Void && f->name.text != "main" && !returns(f->body))
                fail(f->name.loc, "semantic",
                     "non-void function must structurally return on all paths");
            inFunction = false;
        }
        scopes.pop_back();
    }
    for (auto &name : called)
        if (!definitions.contains(name))
            fail(functions[name]->name.loc, "semantic",
                 "called function has no definition: " + name);
    if (!p.script && !definitions.contains("main"))
        fail(p.items.empty()        ? Location{"<input>"}
             : p.items[0].statement ? p.items[0].statement->token.loc
                                    : p.items[0].function->name.loc,
             "semantic", "missing main definition");
}
} // namespace praxis
