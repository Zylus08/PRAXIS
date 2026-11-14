#include "praxis/frontend/parser.hpp"
#include <map>
namespace praxis {
const Token &Parser::peek(size_t n) const {
    return tokens[std::min(pos + n, tokens.size() - 1)];
}
Token Parser::take() {
    auto t = peek();
    if (t.kind != TokenKind::End)
        ++pos;
    return t;
}
bool Parser::match(const std::string &s) {
    if (peek().is(s)) {
        take();
        return true;
    }
    return false;
}
Token Parser::expect(const std::string &s) {
    if (!peek().is(s))
        fail(peek().loc, "parse", "expected '" + s + "', got '" + peek().text + "'");
    return take();
}
Token Parser::identifier() {
    if (peek().kind != TokenKind::Identifier)
        fail(peek().loc, "parse", "expected identifier");
    return take();
}
bool Parser::typeStart() const {
    if (peek().kind != TokenKind::Keyword && peek().kind != TokenKind::Identifier)
        return false;
    auto s = peek().text;
    return s == "const" || s == "bool" || s == "char" || s == "int" || s == "long" ||
           s == "float" || s == "double" || s == "void" ||
           (s == "std" && peek(1).is("::") && peek(2).is("string"));
}
Type Parser::type() {
    Type t;
    t.constant = match("const");
    auto tok = take();
    auto s = tok.text;
    if (s == "bool")
        t.base = Base::Bool;
    else if (s == "char")
        t.base = Base::Char;
    else if (s == "int")
        t.base = Base::Int;
    else if (s == "long") {
        expect("long");
        t.base = Base::Long;
    } else if (s == "float")
        t.base = Base::Float;
    else if (s == "double")
        t.base = Base::Double;
    else if (s == "void")
        t.base = Base::Void;
    else if (s == "std") {
        expect("::");
        expect("string");
        t.base = Base::String;
    } else
        fail(tok.loc, "parse", "expected supported type");
    if (match("const"))
        t.constant = true;
    t.reference = match("&");
    return t;
}
Decl Parser::declaration(Type t, Token name) {
    Decl d{name, t, {}, {}, false};
    if (match("[")) {
        auto size = take();
        if (size.kind != TokenKind::Integer)
            fail(size.loc, "parse", "array bound must be a positive decimal literal");
        try {
            size_t used = 0;
            auto n = std::stoll(size.text, &used);
            if (used != size.text.size() || n < 1 || n > 1000000)
                throw std::out_of_range("bound");
            d.type.extent = static_cast<int>(n);
        } catch (...) {
            fail(size.loc, "parse", "invalid array bound (1..1000000)");
        }
        expect("]");
    }
    if (match("=")) {
        if (match("{")) {
            d.braced = true;
            if (!match("}")) {
                do {
                    d.elements.push_back(expression());
                } while (match(",") && !peek().is("}"));
                expect("}");
            }
        } else
            d.init = expression();
    }
    expect(";");
    return d;
}
S Parser::block() {
    auto t = expect("{");
    auto s = std::make_shared<Stmt>(Stmt::Kind::Block, t);
    while (!peek().is("}")) {
        if (peek().kind == TokenKind::End)
            fail(peek().loc, "parse", "unterminated block");
        s->children.push_back(statement());
    }
    expect("}");
    return s;
}
S Parser::statement() {
    if (++depth > 256)
        fail(peek().loc, "parse", "nesting limit");
    struct Guard {
        int &d;
        ~Guard() {
            --d;
        }
    } guard{depth};
    auto t = peek();
    if (t.is("{"))
        return block();
    if (typeStart()) {
        auto ty = type();
        auto n = identifier();
        auto s = std::make_shared<Stmt>(Stmt::Kind::Declaration, t);
        s->decl = declaration(ty, n);
        return s;
    }
    if (match(";"))
        return std::make_shared<Stmt>(Stmt::Kind::Empty, t);
    if (match("if")) {
        auto s = std::make_shared<Stmt>(Stmt::Kind::If, t);
        expect("(");
        s->condition = expression();
        expect(")");
        s->children.push_back(statement());
        if (match("else"))
            s->children.push_back(statement());
        return s;
    }
    if (match("while")) {
        auto s = std::make_shared<Stmt>(Stmt::Kind::While, t);
        expect("(");
        s->condition = expression();
        expect(")");
        s->children.push_back(statement());
        return s;
    }
    if (match("do")) {
        auto s = std::make_shared<Stmt>(Stmt::Kind::Do, t);
        s->children.push_back(statement());
        expect("while");
        expect("(");
        s->condition = expression();
        expect(")");
        expect(";");
        return s;
    }
    if (match("for")) {
        auto s = std::make_shared<Stmt>(Stmt::Kind::For, t);
        expect("(");
        s->children.push_back(statement());
        if (!peek().is(";"))
            s->condition = expression();
        expect(";");
        if (!peek().is(")"))
            s->step = expression();
        expect(")");
        s->children.push_back(statement());
        return s;
    }
    if (match("return")) {
        auto s = std::make_shared<Stmt>(Stmt::Kind::Return, t);
        if (!peek().is(";"))
            s->expr = expression();
        expect(";");
        return s;
    }
    if (match("break") || match("continue")) {
        auto s =
            std::make_shared<Stmt>(t.is("break") ? Stmt::Kind::Break : Stmt::Kind::Continue, t);
        expect(";");
        return s;
    }
    auto s = std::make_shared<Stmt>(Stmt::Kind::Expression, t);
    s->expr = expression();
    expect(";");
    return s;
}
int Parser::precedence(const std::string &s) const {
    static const std::map<std::string, int> p = {
        {"=", 1},  {"+=", 1}, {"-=", 1}, {"*=", 1}, {"/=", 1}, {"%=", 1}, {"||", 2},
        {"&&", 3}, {"==", 4}, {"!=", 4}, {"<", 5},  {">", 5},  {"<=", 5}, {">=", 5},
        {"<<", 6}, {">>", 6}, {"+", 7},  {"-", 7},  {"*", 8},  {"/", 8},  {"%", 8}};
    auto it = p.find(s);
    return it == p.end() ? 0 : it->second;
}
E Parser::expression(int minimum) {
    if (++depth > 256)
        fail(peek().loc, "parse", "expression nesting limit");
    struct Guard {
        int &d;
        ~Guard() {
            --d;
        }
    } guard{depth};
    auto a = prefix();
    while (true) {
        auto t = peek();
        int p = t.kind == TokenKind::Symbol ? precedence(t.text) : 0;
        if (p < minimum)
            break;
        take();
        auto b = expression(p + (p == 1 ? 0 : 1));
        auto e =
            std::make_shared<Expr>(p == 1 ? Expr::Kind::Assign : Expr::Kind::Binary, t, t.text);
        e->args = {a, b};
        e->height = 1 + std::max(a->height, b->height);
        if (e->height > 128)
            fail(t.loc, "parse", "expression tree depth limit");
        a = e;
    }
    return a;
}
E Parser::prefix() {
    auto t = peek();
    if (t.is("+") || t.is("-") || t.is("!") || t.is("++") || t.is("--")) {
        take();
        auto e = std::make_shared<Expr>(Expr::Kind::Unary, t, t.text);
        e->args = {expression(9)};
        e->height = e->args[0]->height + 1;
        if (e->height > 128)
            fail(t.loc, "parse", "expression tree depth limit");
        return e;
    }
    E e;
    if (match("(")) {
        e = expression();
        expect(")");
    } else if (t.kind == TokenKind::Integer || t.kind == TokenKind::Floating ||
               t.kind == TokenKind::String || t.kind == TokenKind::Character || t.is("true") ||
               t.is("false")) {
        take();
        e = std::make_shared<Expr>(Expr::Kind::Literal, t);
    } else if (t.kind == TokenKind::Identifier) {
        take();
        if (match("::")) {
            t.text += "::" + identifier().text;
        }
        e = std::make_shared<Expr>(Expr::Kind::Name, t, t.text);
    } else
        fail(t.loc, "parse", "expected expression");
    return postfix(e);
}
E Parser::postfix(E e) {
    while (true) {
        auto t = peek();
        if (match("(")) {
            auto c = std::make_shared<Expr>(Expr::Kind::Call, t);
            c->args.push_back(e);
            if (!match(")")) {
                do {
                    c->args.push_back(expression());
                } while (match(","));
                expect(")");
            }
            e = c;
        } else if (match("[")) {
            auto c = std::make_shared<Expr>(Expr::Kind::Index, t);
            c->args = {e, expression()};
            expect("]");
            e = c;
        } else if (match(".")) {
            auto n = identifier();
            auto c = std::make_shared<Expr>(Expr::Kind::Member, n, n.text);
            c->args = {e};
            expect("(");
            expect(")");
            e = c;
        } else if (match("++") || match("--")) {
            auto c = std::make_shared<Expr>(Expr::Kind::Postfix, t, t.text);
            c->args = {e};
            e = c;
        } else
            break;
        int height = 1;
        for (auto a : e->args)
            height = std::max(height, a->height + 1);
        e->height = height;
        if (height > 128)
            fail(t.loc, "parse", "expression tree depth limit");
    }
    return e;
}
Program Parser::parse(bool script) {
    Program p;
    p.script = script;
    while (peek().kind != TokenKind::End) {
        if (typeStart()) {
            auto t = type();
            auto name = identifier();
            if (match("(")) {
                auto f = std::make_shared<Function>();
                f->name = name;
                f->result = t;
                if (!match(")")) {
                    if (peek().is("void") && peek(1).is(")")) {
                        take();
                        take();
                    } else {
                        do {
                            auto pt = type();
                            auto pn = identifier();
                            f->params.push_back({pn, pt, {}, {}, false});
                        } while (match(","));
                        expect(")");
                    }
                }
                if (!match(";"))
                    f->body = block();
                p.items.push_back({{}, f});
            } else {
                auto s = std::make_shared<Stmt>(Stmt::Kind::Declaration, name);
                s->decl = declaration(t, name);
                p.items.push_back({s, {}});
            }
        } else if (script)
            p.items.push_back({statement(), {}});
        else
            fail(peek().loc, "parse", "expected global declaration or function");
    }
    return p;
}
} // namespace praxis
