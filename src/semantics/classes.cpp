#include "praxis/semantics/analyzer.hpp"
#include <algorithm>
#include <optional>
#include "praxis/common/timing.hpp"
namespace praxis {
Type Analyzer::resolve(Type t,const Location& l){
    if(t.base==Base::Parameter)fail(l,"semantic","unsubstituted template type: "+t.name);
    if(t.base!=Base::Record)return t;
    if(t.name=="std::vector" || t.name=="std::pair" || t.name=="std::array") {
        for(auto& a:t.arguments) a=resolve(a,l);
        return t;
    }
    if(t.pointer > 0) return t;
    for(auto& a:t.arguments){a=resolve(a,l);if(a.reference||a.constant||a.extent||a.base==Base::Void)fail(l,"semantic","unsupported template type argument");}
    auto c=record(t,l);t.name=c->type.name;t.arguments=c->type.arguments;return t;
}
std::shared_ptr<Class> Analyzer::record(Type t,const Location& l){
    if(t.base!=Base::Record||t.extent)fail(l,"semantic","member operation requires a class object");
    t.constant=false;t.reference=false;auto key=typeName(t);
    if(program->compiledClasses.contains(key)){if(!t.arguments.empty())++program->cacheHits;return program->compiledClasses.at(key);}
    if(t.name=="std::vector" || t.name=="std::pair" || t.name=="std::array"){
        auto c = std::make_shared<Class>();
        c->key = key;
        c->name = Token{TokenKind::Identifier, t.name, l};
        c->type = t;
        c->assignable = true;
        program->compiledClasses[key] = c;
        return c;
    }
    if(!classPatterns.contains(t.name))fail(l,"semantic","unknown class type: "+t.name);
    auto pattern=classPatterns.at(t.name);
    if(pattern->templateParameters.size()!=t.arguments.size())fail(l,"semantic","incorrect class template argument count");
    if(pattern->templateParameters.empty()){auto c=cloneClass(pattern,{});c->type=t;c->key=key;defineClass(c);return c;}
    if(templateLevel>=limits.templateDepth||program->instantiations>=limits.instantiations)fail(l,"semantic","template instantiation limit");
    Timer timer(program->templateNanoseconds);++templateLevel;++program->instantiations;Substitution map;map[t.name]=t;
    for(size_t i=0;i<t.arguments.size();++i)map[pattern->templateParameters[i]]=t.arguments[i];
    auto c=cloneClass(pattern,map);c->type=t;c->key=key;c->templateParameters.clear();for(auto f:c->methods)f->instantiated=true;
    try{defineClass(c);}catch(const Error& e){--templateLevel;fail(l,"semantic","instantiating "+key+": "+e.what());}
    --templateLevel;return c;
}
void Analyzer::defineClass(const std::shared_ptr<Class>& c){
    if(c->name.text=="std")fail(c->name.loc,"semantic","std is reserved for native bindings");
    if(buildingClasses.size()>=limits.templateDepth)fail(c->name.loc,"semantic","class nesting limit");
    program->compiledClasses[c->key]=c;buildingClasses.insert(c->key);
    std::set<std::string> names;
    for(auto& d:c->fields){
        if(!names.insert(d.name.text).second||d.name.text==c->name.text)fail(d.name.loc,"semantic","duplicate or invalid data member");
        if(d.type.reference||d.type.extent||d.direct)fail(d.name.loc,"semantic","reference/array members and parenthesized in-class data initializers are unsupported");
        auto raw=d.type;raw.constant=false;raw.reference=false;if(d.type.pointer==0 && buildingClasses.contains(typeName(raw)))fail(d.name.loc,"semantic","recursive by-value class member");
        d.type=resolve(d.type,d.name.loc);
        if(d.type.base==Base::Void)fail(d.name.loc,"semantic","void data member");
        if(d.type.base==Base::String&&!d.name.stringAvailable)fail(d.name.loc,"semantic","std::string requires <string>");
        if(d.type.constant)c->assignable=false;
        if(d.type.base==Base::Record && d.type.pointer==0){auto member=record(d.type,d.name.loc);if(!member->assignable)c->assignable=false;c->nestingDepth=std::max(c->nestingDepth,member->nestingDepth+1);if(c->nestingDepth>limits.templateDepth)fail(d.name.loc,"semantic","nested object depth limit");}
    }
    bool hasConstructor=false,hasCopy=false;size_t destructors=0;
    for(auto f:c->methods){f->owner=c->key;
        if(!f->constructor&&!f->destructor&&f->name.text==c->name.text)fail(f->name.loc,"semantic","constructor cannot have a return type");
        if(!f->constructor&&!f->destructor&&names.contains(f->name.text))fail(f->name.loc,"semantic","member function conflicts with data member");
        if(f->constructor){hasConstructor=true;
            if(f->params.size()==1){auto t=f->params[0].type;t.constant=false;t.reference=false;
                if(t==c->type){if(!f->params[0].type.reference||!f->params[0].type.constant)fail(f->name.loc,"semantic","copy constructors require const class& in this subset");hasCopy=true;}}
        }
        if(f->destructor&&++destructors>1)fail(f->name.loc,"semantic","duplicate destructor");
        registerFunction(f);if(f->destructor)c->destructor=f->id;
    }
    auto synth=[&](Function::Special special){auto f=std::make_shared<Function>();f->name=c->name;f->owner=c->key;f->constructor=true;f->special=special;if(special==Function::Special::CopyConstructor){auto t=c->type;t.reference=true;t.constant=true;auto n=c->name;n.text="$source";f->params.push_back({n,t});}registerFunction(f);c->methods.push_back(f);return f;};
    if(!hasConstructor)synth(Function::Special::DefaultConstructor);
    if(!hasCopy)c->copyConstructor=synth(Function::Special::CopyConstructor)->id;
    else for(auto f:c->methods)if(f->constructor&&f->params.size()==1&&f->params[0].type.reference&&f->params[0].type.base==Base::Record){auto t=f->params[0].type;t.constant=false;t.reference=false;if(t==c->type)c->copyConstructor=f->id;}
    buildingClasses.erase(c->key);
    for(auto f:c->methods)if(f->body&&!f->instantiated)checkFunction(f);
}
Analyzer::F Analyzer::constructor(Type t,const std::vector<E>& args,const Location& l){
    auto c=record(t,l);std::vector<F> choices;for(auto f:c->methods)if(f->constructor)choices.push_back(program->compiledFunctions.at(f->id));return select(choices,args,l);
}
void Analyzer::checkDestructor(Type t,const Location& l){auto c=record(t,l);if(!c->destructor.empty()){auto f=program->compiledFunctions.at(c->destructor);checkAccess(f,l);called.insert(f->id);if(f->instantiated)checkFunction(f);}}
void Analyzer::classDeclaration(Decl& d){
    if(d.type.extent)fail(d.name.loc,"semantic","arrays of class objects are unsupported");
    checkDestructor(d.type,d.name.loc);
    if(d.type.reference){if(!d.init)fail(d.name.loc,"semantic","reference requires initializer");expr(d.init);convert(d.init,d.type);return;}
    if(d.braced)fail(d.name.loc,"semantic","class list initialization is unsupported; use parentheses");
    if(d.init){expr(d.init);convert(d.init,d.type);planCopy(d.init,d.type);d.constructor=d.init->copyTarget;}
    else {for(auto a:d.arguments)expr(a);
        if(d.arguments.size()==1&&!d.arguments[0]->lvalue&&d.arguments[0]->type.base==Base::Record){auto a=d.arguments[0]->type,b=d.type;a.constant=false;b.constant=false;if(a==b){d.init=d.arguments[0];d.arguments.clear();d.direct=false;return;}}
        d.constructor=constructor(d.type,d.arguments,d.name.loc)->id;}
}
Type Analyzer::memberExpression(const E& e){
    auto t=expr(e->args[0]);
    if(t.base==Base::Record && t.name=="std::pair"){
        if(e->op=="first"){e->lvalue=e->args[0]->lvalue;return t.arguments[0];}
        if(e->op=="second"){e->lvalue=e->args[0]->lvalue;return t.arguments[1];}
        fail(e->token.loc,"semantic","std::pair member must be 'first' or 'second'");
    }
    auto c=record(t,e->token.loc);
    for(auto& d:c->fields)if(d.name.text==e->op){if(!d.publicAccess&&currentClass!=c->key)fail(e->token.loc,"semantic","private data member is inaccessible");auto r=d.type;r.constant=r.constant||t.constant;e->lvalue=e->args[0]->lvalue;if(!e->lvalue&&r.base==Base::Record)fail(e->token.loc,"semantic","class-valued members of temporary objects are unsupported");return r;}
    fail(e->token.loc,"semantic","unknown data member: "+e->op);
}
void Analyzer::checkFunction(const F& f){
    if(checked.contains(f->id)||checking.contains(f->id))return;
    if(!f->body&&f->special==Function::Special::User)return;
    if(f->instantiated&&templateLevel>=limits.templateDepth)fail(f->name.loc,"semantic","template instantiation depth limit");
    std::optional<Timer> timer;if(f->instantiated)timer.emplace(program->templateNanoseconds);
    checking.insert(f->id);if(f->instantiated)++templateLevel;
    auto oldScopes=scopes;auto oldResult=result;auto oldClass=currentClass;bool oldConst=constMethod,oldFunction=inFunction;int oldLoops=loops;
    scopes={oldScopes[0],{}};result=f->result;currentClass=f->owner;constMethod=f->constMember;inFunction=true;loops=0;
    try{
        for(auto& d:f->params)bind(d.name,d.type);
        if(f->constructor){
            auto c=program->compiledClasses.at(f->owner);
            if(f->special==Function::Special::CopyConstructor){
                for(auto& d:c->fields)if(d.type.base==Base::Record && d.type.pointer==0){auto e=std::make_shared<Expr>(Expr::Kind::Name,d.name);e->type=d.type;e->type.constant=true;e->lvalue=true;constructor(d.type,{e},d.name.loc);checkDestructor(d.type,d.name.loc);}
            }else{
                std::set<std::string> seen;
                for(auto& init:f->initializers){if(!seen.insert(init.name.text).second)fail(init.name.loc,"semantic","duplicate member initializer");bool found=false;for(auto& d:c->fields)found=found||d.name.text==init.name.text;if(!found)fail(init.name.loc,"semantic","unknown member initializer (delegation unsupported)");}
                // Build a complete declaration-ordered initialization plan, including defaults.
                std::vector<MemberInit> ordered;
                for(auto& d:c->fields){MemberInit init;init.name=d.name;bool explicitInit=false;
                    for(auto& i:f->initializers)if(i.name.text==d.name.text){init=i;explicitInit=true;break;}
                    if(!explicitInit&&d.init)init.args={cloneExpr(d.init,{})};
                    for(auto a:init.args)expr(a);
                    if(d.type.base==Base::Record && d.type.pointer==0){checkDestructor(d.type,d.name.loc);bool elide=false;if(init.args.size()==1&&!init.args[0]->lvalue){auto a=init.args[0]->type,b=d.type;a.constant=false;b.constant=false;elide=a==b;}init.constructor=elide?"$elide":constructor(d.type,init.args,d.name.loc)->id;init.valueInitialize=explicitInit&&init.args.empty();}
                    else {if(init.args.size()>1)fail(d.name.loc,"semantic","scalar member takes at most one initializer");if(!init.args.empty())convert(init.args[0],d.type);else if(d.type.constant&&d.type.base!=Base::String&&!explicitInit)fail(d.name.loc,"semantic","const member requires initializer");if(explicitInit&&init.args.empty())init.constructor="$zero";}
                    ordered.push_back(init);
                }
                f->initializers=std::move(ordered);
            }
        }
        if(f->body){stmt(f->body,false);if(result.base!=Base::Void&&f->name.text!="main"&&!returns(f->body))fail(f->name.loc,"semantic","non-void function must structurally return on all paths");}
    }catch(...){scopes=oldScopes;result=oldResult;currentClass=oldClass;constMethod=oldConst;inFunction=oldFunction;loops=oldLoops;if(f->instantiated)--templateLevel;checking.erase(f->id);throw;}
    scopes=oldScopes;result=oldResult;currentClass=oldClass;constMethod=oldConst;inFunction=oldFunction;loops=oldLoops;if(f->instantiated)--templateLevel;checking.erase(f->id);checked.insert(f->id);
}
}
