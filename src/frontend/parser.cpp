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
           (s == "std" && peek(1).is("::") && peek(2).is("string")) || knownTypes.contains(s) || templateTypes.contains(s);
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
    } else if(templateTypes.contains(s)){t.base=Base::Parameter;t.name=s;}
    else if(knownTypes.contains(s)){t.base=Base::Record;t.name=s;if(peek().is("<"))t.arguments=typeArguments();}
    else fail(tok.loc, "parse", "expected supported type");
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
    if(match("(")){d.direct=true;if(!match(")")){do{d.arguments.push_back(expression());}while(match(","));expect(")");}}
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
    if (typeStart() && !(peek().kind==TokenKind::Identifier && peek(1).is("("))) {
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
        if(peek().is("<")){size_t saved=pos;take();bool types=typeStart();pos=saved;if(types)e->typeArguments=typeArguments();}
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

std::vector<Type> Parser::typeArguments(){
    expect("<");std::vector<Type> a;
    if(peek().is(">"))fail(peek().loc,"parse","empty template argument list unsupported");
    do{a.push_back(type());if(a.size()>8)fail(peek().loc,"parse","template argument limit");}while(match(","));
    if(peek().is(">>")){auto t=take();t.text=">";tokens.insert(tokens.begin()+static_cast<ptrdiff_t>(pos),t);}
    else expect(">");return a;
}
std::vector<std::string> Parser::templateParameters(){
    expect("template");expect("<");std::vector<std::string> names;
    do{if(!match("typename")&&!match("class"))fail(peek().loc,"parse","only template type parameters are supported");auto n=identifier();if(templateTypes.contains(n.text))fail(n.loc,"parse","duplicate template parameter");templateTypes.insert(n.text);names.push_back(n.text);if(names.size()>8)fail(n.loc,"parse","template parameter limit");}while(match(","));expect(">");return names;
}
std::vector<Decl> Parser::parameters(){
    expect("(");std::vector<Decl> out;
    if(match(")"))return out;
    if(peek().is("void")&&peek(1).is(")")){take();take();return out;}
    do{auto t=type();auto n=identifier();out.push_back({n,t});}while(match(","));expect(")");return out;
}
bool Parser::functionAhead()const{
    if(!peek().is("("))return false;
    if(peek(1).is(")"))return true;
    const auto& t=peek(1);return t.kind==TokenKind::Keyword || knownTypes.contains(t.text)||templateTypes.contains(t.text)||(t.is("std")&&peek(2).is("::"));
}
void Parser::functionBody(const std::shared_ptr<Function>& f){
    f->constMember=match("const");
    if(match(":")){
        if(!f->constructor)fail(f->name.loc,"parse","initializer list requires constructor");
        do{MemberInit init;init.name=identifier();expect("(");if(!match(")")){do{init.args.push_back(expression());}while(match(","));expect(")");}f->initializers.push_back(init);}while(match(","));
    }
    if(!match(";"))f->body=block();
}
std::shared_ptr<Function> Parser::function(Type t,Token n,std::string owner){
    auto f=std::make_shared<Function>();f->name=n;f->result=t;f->owner=owner;f->params=parameters();functionBody(f);return f;
}
std::shared_ptr<Class> Parser::classDeclaration(){
    bool access=take().is("struct");auto c=std::make_shared<Class>();c->name=identifier();knownTypes.insert(c->name.text);
    if(peek().is(":"))fail(peek().loc,"parse","inheritance is unsupported");
    expect("{");
    while(!match("}")){
        if(peek().kind==TokenKind::End)fail(peek().loc,"parse","unterminated class");
        if(peek().is("public")||peek().is("private")){access=take().is("public");expect(":");continue;}
        if(peek().is("protected")||peek().is("virtual")||peek().is("static")||peek().is("template")||peek().is("friend"))fail(peek().loc,"parse","unsupported class member feature");
        if((peek().is(c->name.text)&&peek(1).is("("))||peek().is("~")){
            auto f=std::make_shared<Function>();f->owner=c->name.text;f->publicAccess=access;f->destructor=match("~");f->constructor=!f->destructor;f->name=expect(c->name.text);f->params=parameters();functionBody(f);c->methods.push_back(f);continue;
        }
        auto t=type();auto n=identifier();
        if(functionAhead()){auto f=function(t,n,c->name.text);f->publicAccess=access;c->methods.push_back(f);}
        else{auto d=declaration(t,n);d.publicAccess=access;c->fields.push_back(d);}
    }
    expect(";");return c;
}
Program Parser::parse(bool script) {
    Program p;p.script=script;
    while(peek().kind!=TokenKind::End){
        auto oldParams=templateTypes;std::vector<std::string> params;if(peek().is("template"))params=templateParameters();
        if(peek().is("class")||peek().is("struct")){auto c=classDeclaration();c->templateParameters=params;p.items.push_back({{},{},c});}
        else if(typeStart()){
            auto t=type();auto name=identifier();std::string owner;
            if(match("::")){owner=name.text;name=identifier();}
            if(functionAhead()) {auto f=function(t,name,owner);f->templateParameters=params;p.items.push_back({{},f,{}});}
            else {if(!params.empty()||!owner.empty())fail(name.loc,"parse","expected function declaration");auto st=std::make_shared<Stmt>(Stmt::Kind::Declaration,name);st->decl=declaration(t,name);p.items.push_back({st,{},{}});}
        }else if(script&&params.empty())p.items.push_back({statement(),{},{}});
        else fail(peek().loc,"parse","expected global declaration, class or function");
        templateTypes=oldParams;
    }
    return p;
}
} // namespace praxis
