#include "praxis/runtime/interpreter.hpp"
#include "praxis/native/bindings.hpp"
#include <functional>
#include <set>
#include <exception>
#include "praxis/common/timing.hpp"
namespace praxis {
namespace {
void expire(const std::shared_ptr<Object> &o) {
    o->alive = false;
    for (auto e : o->elements) expire(e);
    for (auto e : o->members) expire(e);
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
    if (++steps > limits.steps && !(cleanupDepth>0&&++cleanupReserve<=10000))
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
    if(liveObjects>=limits.objects)fail({"<runtime>"},"runtime","live object limit exceeded");
    auto o = std::make_shared<Object>();++liveObjects;++profile.objects;profile.peakObjects=std::max(profile.peakObjects,liveObjects);
    o->type = t;
    o->type.reference = false;
    o->value = zero(t);
    o->initialized = initialized || t.base == Base::String;
    if(t.base==Base::Record){auto key=t;key.constant=false;key.reference=false;key.extent=0;o->definition=classes.at(typeName(key));o->initialized=false;}
    if (t.extent) {
        Type element = t;
        element.extent = 0;
        try{for (int i = 0; i < t.extent; ++i)o->elements.push_back(allocate(element, initialized));}
        catch(...){auto primary=std::current_exception();try{destroy(o);}catch(...){}std::rethrow_exception(primary);}
        o->initialized = true;
    }
    return o;
}
Value Interpreter::value(const E &e) {
    return read(eval(e), e->token.loc);
}
Evaluation Interpreter::eval(const E &e) {
    if (evalDepth >= 256+(cleanupDepth>0?16:0))
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
        if(e->implicitMember){auto self=frames.back()->self;requireAlive(self,l);auto it=self->fields.find(e->op);if(it==self->fields.end())fail(l,"runtime","member is not initialized yet: "+e->op);return {{},it->second};}
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
    case Expr::Kind::Member:{auto owner=object(eval(e->args[0]),l);auto it=owner->fields.find(e->op);if(it==owner->fields.end())fail(l,"runtime","member is not initialized yet: "+e->op);return {{},it->second};}
    case Expr::Kind::Construct:{std::vector<Evaluation> args;for(auto a:e->args)args.push_back(eval(a));auto o=allocate(e->type,false);o->location=l;o->zeroInitialize=args.empty()&&functions.at(e->target)->special==Function::Special::DefaultConstructor;construct(o,e->target,args);temporaries.push_back(o);return {{e->type,o},{}};}
    case Expr::Kind::Call:{
        if(e->target.starts_with("native:"))return {stringMember(e->target.substr(7),value(e->args[0]->args[0])),{}};
        auto f=functions.at(e->target);O self;
        if(!f->owner.empty()){self=e->implicitMember?frames.back()->self:object(eval(e->args[0]->args[0]),l);requireAlive(self,l);}
        std::vector<Evaluation> args;
        for(size_t i=1;i<e->args.size();++i){auto a=eval(e->args[i]);
            if(f->params[i-1].type.base==Base::Record&&!f->params[i-1].type.reference){auto source=object(a,l);if(e->args[i]->lvalue){auto copied=copyObject(source,f->params[i-1].type,e->args[i]->copyTarget,l);temporaries.push_back(copied);a={{f->params[i-1].type,copied},{}};}else a={{f->params[i-1].type,source},{}};}
            else if(!f->params[i-1].type.reference){a.value=read(a,l);a.object.reset();}
            args.push_back(a);
        }
        auto v=call(f,args,self);if(v.type.base==Base::Record)temporaries.push_back(std::get<O>(v.data));return {v,{}};
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
        if(e->type.base==Base::Record){assignObject(a.object,std::get<O>(b.data),l);return a;}
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
    if(d.type.base==Base::Record){O o;
        if(d.init){auto a=eval(d.init);auto source=object(a,d.name.loc);if(!d.init->lvalue){o=source;detachTemporary(o);o->type=d.type;}else o=copyObject(source,d.type,d.constructor,d.name.loc);}
        else {std::vector<Evaluation> args;for(auto e:d.arguments)args.push_back(eval(e));o=allocate(d.type,false);o->location=d.name.loc;o->zeroInitialize=frames.empty();construct(o,d.constructor,args);}
        scope.names[d.name.text]=o;scope.owned.push_back(o);return;
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
Flow Interpreter::nested(const S& s){
    auto frame=frames.back().get();frame->scopes.push_back(std::make_unique<Scope>());
    Flow flow;try{flow=execute(s,false);cleanup(*frame->scopes.back());frame->scopes.pop_back();return flow;}
    catch(...){auto primary=std::current_exception();try{cleanup(*frame->scopes.back());}catch(...){}frame->scopes.pop_back();try{discardReturn(flow);}catch(...){}std::rethrow_exception(primary);}
}
Flow Interpreter::execute(const S& s,bool scoped){size_t mark=temporaries.size();Flow flow;try{flow=executeInner(s,scoped);cleanupTemporaries(mark);return flow;}catch(...){auto primary=std::current_exception();try{cleanupTemporaries(mark);}catch(...){}try{discardReturn(flow);}catch(...){}std::rethrow_exception(primary);}}
Flow Interpreter::executeInner(const S& s,bool scoped){
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
        if (test(s->condition))
            return nested(s->children[0]);
        else if (s->children.size() > 1)
            return nested(s->children[1]);
        break;
    case Stmt::Kind::While:
    case Stmt::Kind::Do: {
        bool first = s->kind == Stmt::Kind::Do;
        while (first || test(s->condition)) {
            first = false;
            auto f = nested(s->children[0]);
            if (f.kind == Flow::Return)
                return f;
            if (f.kind == Flow::Break)
                break;
        }
        break;
    }
    case Stmt::Kind::For:{
        auto frame=frames.back().get();frame->scopes.push_back(std::make_unique<Scope>());Flow result;
        try{execute(s->children[0]);while(!s->condition||test(s->condition)){auto f=nested(s->children[1]);if(f.kind==Flow::Return){result=f;break;}if(f.kind==Flow::Break)break;if(s->step)discard(s->step);tick(s->token.loc);}cleanup(*frame->scopes.back());frame->scopes.pop_back();return result;}
        catch(...){auto primary=std::current_exception();try{cleanup(*frame->scopes.back());}catch(...){}frame->scopes.pop_back();try{discardReturn(result);}catch(...){}std::rethrow_exception(primary);}
    }
    case Stmt::Kind::Return:{
        if(!s->expr)return {Flow::Return,zero({Base::Void})};
        if(s->expr->type.base==Base::Record){auto source=object(eval(s->expr),s->token.loc);O returned;
            if(s->expr->lvalue)returned=copyObject(source,frames.back()->function->result,s->expr->copyTarget,s->token.loc);
            else {returned=source;detachTemporary(returned);}
            return {Flow::Return,{frames.back()->function->result,returned}};
        }
        return {Flow::Return,value(s->expr)};
    }
    case Stmt::Kind::Break:
        return {Flow::Break, {}};
    case Stmt::Kind::Continue:
        return {Flow::Continue, {}};
    case Stmt::Kind::Empty:
        break;
    }
    return {};
}
Value Interpreter::call(const std::shared_ptr<Function>& f,const std::vector<Evaluation>& args,O self){
    if(frames.size()>=limits.calls+(cleanupDepth>0?16:0))fail(f->name.loc,"runtime","call depth limit exceeded");
    Timer timer(profile.callNanoseconds);++profile.calls;auto frame=std::make_unique<Frame>();frame->self=self;frame->function=f;frame->scopes.push_back(std::make_unique<Scope>());frames.push_back(std::move(frame));auto active=frames.back().get();Value returned=zero(f->result);
    try{
        for(size_t i=0;i<args.size();++i){auto& d=f->params[i];
            if(d.type.reference){auto o=object(args[i],d.name.loc);current().names[d.name.text]=o;}
            else if(d.type.base==Base::Record){auto o=object(args[i],d.name.loc);if(args[i].object)o=copyObject(o,d.type,"",d.name.loc);else detachTemporary(o);current().names[d.name.text]=o;current().owned.push_back(o);}
            else{auto converted=convertValue(read(args[i],d.name.loc),d.type,d.name.loc);auto o=allocate(d.type,true);o->value=converted;current().owned.push_back(o);current().names[d.name.text]=o;}
        }
        if(f->constructor)initializeMembers(self,f,args);
        auto flow=f->body?execute(f->body,false):Flow{};
        if(flow.kind==Flow::Return&&f->result.base!=Base::Void)returned=f->result.base==Base::Record?flow.value:convertValue(flow.value,f->result,f->name.loc);
        cleanup(*active->scopes.back());frames.pop_back();return returned;
    }catch(...){auto primary=std::current_exception();try{cleanup(*active->scopes.back());}catch(...){}frames.pop_back();if(returned.type.base==Base::Record&&std::holds_alternative<O>(returned.data))try{destroy(std::get<O>(returned.data));}catch(...){}std::rethrow_exception(primary);}
}
int Interpreter::run(const Program &p, size_t start) {
    resetBudget();
    functions=p.compiledFunctions;classes=p.compiledClasses;
    try{
    for (size_t i = start; i < p.items.size(); ++i) {
        auto s = p.items[i].statement;
        if (s && s->kind == Stmt::Kind::Declaration && !s->decl.type.reference && s->decl.type.base!=Base::Record) {
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
        if (d.type.reference && d.type.base!=Base::Record)
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
                    execute(s);
            } else {
                auto f = std::make_unique<Frame>();
                f->scopes.push_back(std::make_unique<Scope>());
                frames.push_back(std::move(f));
                try {
                    execute(s);
                    cleanup(*frames.back()->scopes.back());
                    frames.pop_back();
                } catch (...) {
                    try{cleanup(*frames.back()->scopes.back());}catch(...){}
                    frames.pop_back();
                    throw;
                }
            }
        }
    if (p.script)
        return 0;
    int code=static_cast<int>(std::get<std::int64_t>(call(functions.at("main()"),{}).data));finish();return code;
    }catch(...){auto primary=std::current_exception();if(!p.script)try{finish();}catch(...){}std::rethrow_exception(primary);}
}
} // namespace praxis
