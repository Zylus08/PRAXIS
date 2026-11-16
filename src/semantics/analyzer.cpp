#include "praxis/semantics/analyzer.hpp"
#include <cmath>
#include <limits>
namespace praxis {
void Analyzer::bind(const Token &n, Type t) {
    if (n.text == "std")
        fail(n.loc, "semantic", "std is reserved for native bindings");
    if (classPatterns.contains(n.text))fail(n.loc,"semantic","object names hiding class types are unsupported");
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
        if (!e->lvalue || s.extent || (s.base != t.base || s.name!=t.name || s.arguments!=t.arguments || s.pointer != t.pointer) || (!t.constant && s.constant) || (!t.pointeeConstant && s.pointeeConstant))
            fail(e->token.loc, "semantic", "reference requires compatible lvalue");
        return;
    }
    if (s.extent || t.extent || s.base == Base::Void || t.base == Base::Void)
        fail(e->token.loc, "semantic", "invalid value conversion");
    if (t.pointer > 0) {
        if (s.base == Base::Nullptr)
            return;
        if (s.pointer == t.pointer && s.base == t.base && s.name == t.name && s.arguments == t.arguments) {
            if (!t.pointeeConstant && s.pointeeConstant)
                fail(e->token.loc, "semantic", "cannot convert const pointer to non-const pointer");
            return;
        }
        fail(e->token.loc, "semantic", "cannot convert " + typeName(s) + " to pointer type " + typeName(t));
    }
    if (t.base == Base::Bool && (s.pointer > 0 || s.base == Base::Nullptr))
        return;
    if (numeric(s) && numeric(t))
        return;
    if (t.base == Base::String && (s.base == Base::String || s.base == Base::Text))
        return;
    if(t.base==Base::Record){if(s.base==Base::Record&&s.name==t.name&&s.arguments==t.arguments)return;fail(e->token.loc,"semantic","incompatible class types");}
    if (t.base == s.base && t.base != Base::Stream)
        return;
    fail(e->token.loc, "semantic", "cannot convert " + typeName(s) + " to " + typeName(t));
}
void Analyzer::condition(const E &e) {
    if (e) {
        auto t = expr(e);
        if (t.pointer > 0 || t.base == Base::Nullptr)
            return;
        if (!numeric(t))
            fail(e->token.loc, "semantic", "condition requires numeric, bool or pointer value");
    }
}
Type Analyzer::expr(const E &e) {
    auto error = [&](const std::string &m) { fail(e->token.loc, "semantic", m); };
    e->lvalue = false;
    Type t;
    switch (e->kind) {
    case Expr::Kind::Literal: {
        auto &tok = e->token;
        if (tok.text == "nullptr" || e->type.base == Base::Nullptr) {
            t.base = Base::Nullptr;
        } else if (tok.kind == TokenKind::Integer) {
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
        if (e->op == "nullptr") {
            t.base = Base::Nullptr;
        } else if (e->op == "std::cout") {
            if (!e->token.iostreamAvailable)
                error("std::cout requires <iostream>");
            t.base = Base::Stream;
        } else if (e->op == "std::endl") {
            if (!e->token.iostreamAvailable)
                error("std::endl requires <iostream>");
            t.base = Base::Endl;
            t.constant = true;
        } else {
            bool found=false;
            for(size_t i=scopes.size();i>1;--i){auto it=scopes[i-1].find(e->op);if(it!=scopes[i-1].end()){t=it->second;found=true;break;}}
            if(!found&&!currentClass.empty()){auto c=program->compiledClasses.at(currentClass);for(auto& d:c->fields)if(d.name.text==e->op){t=d.type;t.constant=t.constant||constMethod;e->implicitMember=true;found=true;break;}}
            if(!found)t=lookup(e->token);
            t.reference = false;
            e->lvalue = true;
        }
        break;
    case Expr::Kind::Index: {
        auto a = expr(e->args[0]);
        auto b = expr(e->args[1]);
        if (!integral(b))
            error("array/container index must be integral");
        if (a.extent) {
            t = a;
            t.extent = 0;
            e->lvalue = true;
        } else if (a.pointer > 0) {
            t = a;
            t.pointer--;
            t.constant = a.pointeeConstant;
            t.pointeeConstant = false;
            e->lvalue = true;
        } else if (a.base == Base::String || a.base == Base::Text) {
            t.base = Base::Char;
            e->lvalue = (a.base == Base::String && !a.constant);
        } else if (a.base == Base::Record && (a.name == "std::vector" || a.name == "std::array")) {
            if (a.arguments.empty())
                error("container has no element type");
            t = a.arguments[0];
            t.constant = t.constant || a.constant;
            e->lvalue = true;
        } else {
            error("index requires fixed array, pointer, string or vector");
        }
        break;
    }
    case Expr::Kind::Member:t=memberExpression(e);break;
    case Expr::Kind::Call:t=callExpression(e);break;
    case Expr::Kind::Construct:{for(auto a:e->args)expr(a);t=resolve(e->type,e->token.loc);e->target=constructor(t,e->args,e->token.loc)->id;break;}
    case Expr::Kind::New: {
        t = resolve(e->type, e->token.loc);
        Type pointee = t;
        pointee.pointer--;
        pointee.constant = false;
        pointee.reference = false;
        if (e->op == "[]") {
            auto sz = expr(e->args[0]);
            if (!integral(sz))
                error("array new bound must be integral");
        } else if (pointee.base == Base::Record) {
            for (auto a : e->args)
                expr(a);
            e->target = constructor(pointee, e->args, e->token.loc)->id;
        } else {
            if (!e->args.empty()) {
                if (e->args.size() > 1)
                    error("scalar new takes at most one argument");
                expr(e->args[0]);
                convert(e->args[0], pointee);
            }
        }
        e->lvalue = false;
        break;
    }
    case Expr::Kind::Delete: {
        auto a = expr(e->args[0]);
        if (a.pointer <= 0 && a.base != Base::Nullptr)
            error("delete requires pointer operand");
        t.base = Base::Void;
        e->lvalue = false;
        break;
    }
    case Expr::Kind::InitList: {
        for (auto a : e->args)
            expr(a);
        t.base = Base::Void;
        e->lvalue = false;
        break;
    }
    case Expr::Kind::Unary:
    case Expr::Kind::Postfix: {
        if (e->op == "&" && e->kind == Expr::Kind::Unary) {
            auto a = expr(e->args[0]);
            if (!e->args[0]->lvalue || e->args[0]->type.extent)
                error("address-of requires an lvalue");
            t = a;
            t.pointer++;
            t.reference = false;
            t.constant = false;
            t.pointeeConstant = a.constant;
            e->lvalue = false;
            break;
        }
        if (e->op == "*" && e->kind == Expr::Kind::Unary) {
            auto a = expr(e->args[0]);
            if (a.pointer <= 0)
                error("dereference requires pointer operand");
            t = a;
            t.pointer--;
            t.constant = a.pointeeConstant;
            t.pointeeConstant = false;
            t.reference = false;
            e->lvalue = true;
            break;
        }
        auto a = expr(e->args[0]);
        if (e->op == "!" && (a.pointer > 0 || a.base == Base::Nullptr)) {
            t.base = Base::Bool;
            break;
        }
        if (a.pointer > 0 && (e->op == "++" || e->op == "--")) {
            modifiable(e->args[0]);
            t = a;
            e->lvalue = e->kind == Expr::Kind::Unary;
            break;
        }
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
        if(a.base==Base::Record){if(e->op!="=")error("class compound assignment unsupported");convert(e->args[1],a);if(!record(a,e->token.loc)->assignable)error("implicit copy assignment is deleted (const member)");t=a;e->lvalue=true;break;}
        if (a.pointer > 0) {
            if (e->op == "=") {
                convert(e->args[1], a);
            } else if (e->op == "+=" || e->op == "-=") {
                if (!integral(b))
                    error("pointer arithmetic requires integral operand");
            } else {
                error("invalid pointer assignment operator");
            }
            t = a;
            e->lvalue = true;
            break;
        }
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
            if (b.extent || b.base == Base::Void || b.base == Base::Stream || b.base==Base::Record)
                error("unsupported stream operand");
            t = a;
            break;
        }
        bool textA = a.base == Base::String || a.base == Base::Text,
             textB = b.base == Base::String || b.base == Base::Text;
        bool compare =
            op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=";
        bool isPtrA = a.pointer > 0 || a.base == Base::Nullptr;
        bool isPtrB = b.pointer > 0 || b.base == Base::Nullptr;
        if (isPtrA || isPtrB) {
            if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
                t.base = Base::Bool;
                break;
            }
            if (a.pointer > 0 && integral(b) && op == "+") {
                t = a;
                break;
            }
            if (integral(a) && b.pointer > 0 && op == "+") {
                t = b;
                break;
            }
            if (a.pointer > 0 && integral(b) && op == "-") {
                t = a;
                break;
            }
            if (a.pointer > 0 && b.pointer > 0 && op == "-") {
                t.base = Base::Long;
                break;
            }
            if (a.pointer > 0 && b.pointer > 0 && (op == "<" || op == ">" || op == "<=" || op == ">=")) {
                t.base = Base::Bool;
                break;
            }
            error("invalid pointer binary operator");
        }
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
    if (d.type.base == Base::Auto) {
        if (!d.init && d.elements.empty())
            fail(d.name.loc, "semantic", "auto declaration requires initializer");
        Type deduced;
        if (d.init) {
            deduced = expr(d.init);
        } else if (!d.elements.empty()) {
            fail(d.name.loc, "semantic", "auto with braced list unsupported");
        }
        if (d.type.constant)
            deduced.constant = true;
        if (d.type.reference)
            deduced.reference = true;
        d.type = deduced;
    }
    d.type=resolve(d.type,d.name.loc);
    auto t = d.type;
    if (t.base == Base::Void || t.base == Base::Stream)
        fail(d.name.loc, "semantic", "invalid object type");
    if (t.base == Base::String && !d.name.stringAvailable)
        fail(d.name.loc, "semantic", "std::string requires <string>");
    if (t.extent && t.reference)
        fail(d.name.loc, "semantic", "arrays of references are invalid");
    bind(d.name, t);
    if(t.base==Base::Record){
        if (t.name == "std::vector" || t.name == "std::array" || t.name == "std::pair") {
            if (t.name == "std::pair") {
                for (size_t i = 0; i < d.arguments.size() && i < t.arguments.size(); ++i) {
                    expr(d.arguments[i]);
                    convert(d.arguments[i], t.arguments[i]);
                }
                if (d.init) {
                    expr(d.init);
                    convert(d.init, t);
                }
                return;
            }
            if (d.braced || !d.elements.empty()) {
                Type elemTy = t.arguments[0];
                for (auto &el : d.elements) {
                    expr(el);
                    convert(el, elemTy);
                }
            } else if (d.init) {
                expr(d.init);
                convert(d.init, t);
            } else {
                Type elemTy = t.arguments[0];
                for (auto a : d.arguments)
                    expr(a);
                if (d.arguments.size() == 1 && elemTy.base == Base::Record && elemTy.pointer == 0) {
                    auto f = constructor(elemTy, {}, d.name.loc);
                    d.constructor = f->id;
                }
            }
            return;
        }
        classDeclaration(d);
        return;
    }
    if (t.pointer > 0) {
        if (d.init) {
            expr(d.init);
            convert(d.init, t);
        } else if (d.braced && d.elements.size() == 1) {
            expr(d.elements[0]);
            convert(d.elements[0], t);
            d.init = d.elements[0];
            d.elements.clear();
            d.braced = false;
        } else if (t.constant) {
            fail(d.name.loc, "semantic", "const pointer requires initializer");
        }
        return;
    }
    if (d.braced && !t.extent && (numeric(t) || t.base == Base::Nullptr)) {
        if (d.elements.size() == 1) {
            expr(d.elements[0]);
            convert(d.elements[0], t);
            d.init = d.elements[0];
            d.elements.clear();
            d.braced = false;
        } else if (d.elements.empty()) {
            d.braced = false;
        }
    }
    if(d.direct)fail(d.name.loc,"semantic","scalar direct initialization is unsupported");
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
    case Stmt::Kind::RangeFor: {
        scopes.emplace_back();
        auto rangeTy = expr(s->expr);
        Type elemTy;
        if (rangeTy.base == Base::Record && (rangeTy.name == "std::vector" || rangeTy.name == "std::array")) {
            if (rangeTy.arguments.empty())
                fail(s->token.loc, "semantic", "container has no element type");
            elemTy = rangeTy.arguments[0];
        } else if (rangeTy.base == Base::String || rangeTy.base == Base::Text) {
            elemTy.base = Base::Char;
        } else if (rangeTy.extent > 0) {
            elemTy = rangeTy;
            elemTy.extent = 0;
        } else {
            fail(s->token.loc, "semantic", "range-for requires vector, string or array");
        }
        if (s->decl.type.base == Base::Auto) {
            bool isConst = s->decl.type.constant;
            bool isRef = s->decl.type.reference;
            s->decl.type = elemTy;
            s->decl.type.constant = isConst;
            s->decl.type.reference = isRef;
        }
        bind(s->decl.name, s->decl.type);
        ++loops;
        stmt(s->children[0], false);
        --loops;
        scopes.pop_back();
        break;
    }
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
            if(result.base==Base::Record){checkDestructor(result,s->token.loc);planCopy(s->expr,result);}
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
void Analyzer::check(Program& p){
    program=&p;scopes={{}};functions.clear();templates.clear();classPatterns.clear();checked.clear();checking.clear();called.clear();buildingClasses.clear();currentClass.clear();inFunction=false;constMethod=false;loops=0;templateLevel=0;
    p.compiledFunctions.clear();p.compiledClasses.clear();p.instantiations=0;p.cacheHits=0;p.templateNanoseconds=0;
    for(auto& item:p.items){
        if(item.record){auto c=item.record;if(classPatterns.contains(c->name.text)||scopes[0].contains(c->name.text)||functions.contains(c->name.text))fail(c->name.loc,"semantic","duplicate class name");classPatterns[c->name.text]=c;if(c->templateParameters.empty()){Type t{Base::Record};t.name=c->name.text;record(t,c->name.loc);}continue;}
        if(item.statement){stmt(item.statement);continue;}
        auto f=item.function;
        if(!f->templateParameters.empty()){if(!f->owner.empty()||!f->body)fail(f->name.loc,"semantic","template methods/prototypes unsupported");
            Substitution shape;for(size_t i=0;i<f->templateParameters.size();++i){Type t{Base::Parameter};t.name="$"+std::to_string(i);shape[f->templateParameters[i]]=t;}
            for(auto old:templates[f->name.text]){if(old->templateParameters.size()!=f->templateParameters.size()||old->params.size()!=f->params.size())continue;Substitution prior;for(size_t i=0;i<old->templateParameters.size();++i){Type t{Base::Parameter};t.name="$"+std::to_string(i);prior[old->templateParameters[i]]=t;}bool same=true;for(size_t i=0;i<f->params.size();++i)same=same&&substitute(f->params[i].type,shape)==substitute(old->params[i].type,prior);if(same)fail(f->name.loc,"semantic","duplicate function template definition");}
            templates[f->name.text].push_back(f);continue;}
        if(!f->owner.empty()){
            Type t{Base::Record};t.name=f->owner;auto c=record(t,f->name.loc);bool declared=false;
            for(auto m:c->methods)if(m->name.text==f->name.text&&m->params.size()==f->params.size()&&m->constMember==f->constMember){bool same=true;for(size_t i=0;i<m->params.size();++i)same=same&&m->params[i].type==resolve(f->params[i].type,f->name.loc);if(same){f->publicAccess=m->publicAccess;declared=true;break;}}
            if(!declared)fail(f->name.loc,"semantic","out-of-class definition has no matching declaration");
        }
        registerFunction(f);checkFunction(f);
    }
    for(auto id:called){auto f=p.compiledFunctions.at(id);if(!f->body&&f->special==Function::Special::User)fail(f->name.loc,"semantic","called function has no definition: "+f->name.text);}
    if(!p.script){auto main=functions.find("main");if(main==functions.end()||main->second.empty()||!main->second[0]->body)fail({"<input>"},"semantic","missing main definition");}
}
} // namespace praxis
