#include "praxis/semantics/analyzer.hpp"
#include <algorithm>
#include <functional>
#include "praxis/common/timing.hpp"
namespace praxis {
namespace {Type bare(Type t){t.constant=false;t.reference=false;return t;}bool same(Type a,Type b){return bare(a)==bare(b);}}
std::string Analyzer::signature(const F& f)const{
    std::string id=f->owner.empty()?"":f->owner+"::";id+=f->constructor?"$ctor":f->destructor?"$dtor":f->name.text;id+="(";
    for(auto d:f->params){auto t=d.type;if(!t.reference)t.constant=false;id+=typeName(t)+";";}id+=")";if(f->constMember)id+="const";return id;
}
int Analyzer::rank(Type target,const E& e)const{
    auto source=e->type;if(source.extent||target.extent)return -1;
    if(target.reference){if(!e->lvalue||!same(source,target)||(!target.constant&&source.constant))return -1;return 0;}
    if(source.base==Base::Void||target.base==Base::Void)return -1;
    if (target.pointer > 0) {
        if (source.base == Base::Nullptr) return 1;
        if (source.pointer == target.pointer && source.base == target.base && source.name == target.name && source.arguments == target.arguments) {
            if (!target.pointeeConstant && source.pointeeConstant) return -1;
            return target.pointeeConstant == source.pointeeConstant ? 0 : 1;
        }
        return -1;
    }
    if (source.pointer > 0) return -1;
    if(same(source,target))return 0;
    if(numeric(source)&&numeric(target)){
        if((source.base==Base::Bool||source.base==Base::Char)&&target.base==Base::Int)return 1;
        if(source.base==Base::Float&&target.base==Base::Double)return 1;
        return 2;
    }
    if(target.base==Base::String&&source.base==Base::Text)return 3;
    return -1;
}
void Analyzer::registerFunction(const F& f){
    if(f->name.text=="std")fail(f->name.loc,"semantic","std is reserved for native bindings");
    if(f->owner.empty()&&classPatterns.contains(f->name.text))fail(f->name.loc,"semantic","functions hiding class names are unsupported");
    f->result=resolve(f->result,f->name.loc);
    if(f->result.reference||f->result.constant)fail(f->name.loc,"semantic","qualified/reference return types are unsupported");
    if(f->result.base==Base::String&&!f->name.stringAvailable)fail(f->name.loc,"semantic","std::string requires <string>");
    if(f->constMember&&f->owner.empty())fail(f->name.loc,"semantic","const qualification requires a member function");
    if((f->constructor||f->destructor)&&f->constMember)fail(f->name.loc,"semantic","constructor/destructor cannot be const");
    if(f->destructor&&!f->params.empty())fail(f->name.loc,"semantic","destructor cannot take parameters");
    for(auto& d:f->params){d.type=resolve(d.type,d.name.loc);if(d.type.base==Base::Void||d.type.extent)fail(d.name.loc,"semantic","invalid parameter type");if(d.type.base==Base::String&&!d.name.stringAvailable)fail(d.name.loc,"semantic","std::string requires <string>");}
    f->id=signature(f);
    auto key=f->owner.empty()?f->name.text:f->owner+"::"+(f->constructor?"$ctor":f->destructor?"$dtor":f->name.text);
    if(f->owner.empty()&&scopes[0].contains(f->name.text))fail(f->name.loc,"semantic","function conflicts with object");
    auto& set=functions[key];
    for(auto& prior:set)if(signature(prior)==f->id){
        if(prior->result!=f->result)fail(f->name.loc,"semantic","overloads cannot differ only by return type");
        if(prior->body&&f->body)fail(f->name.loc,"semantic","duplicate function definition");
        if(f->body){f->publicAccess=prior->publicAccess;prior=f;program->compiledFunctions[f->id]=f;}return;
    }
    set.push_back(f);program->compiledFunctions[f->id]=f;
    if(f->name.text=="main"&&(f->result.base!=Base::Int||!f->params.empty()||!f->owner.empty()||set.size()>1))fail(f->name.loc,"semantic","main must have signature int main()");
}
Analyzer::F Analyzer::instantiateFunction(const F& pattern,const std::vector<Type>& args,const Location& loc){
    if(args.size()!=pattern->templateParameters.size())fail(loc,"semantic","incorrect template argument count");
    std::string key="template:"+pattern->name.text+"<";Substitution map;
    for(size_t i=0;i<args.size();++i){key+=typeName(args[i])+",";map[pattern->templateParameters[i]]=args[i];}key+=">";
    // Pattern signatures disambiguate overload templates without claiming partial ordering.
    for(auto d:pattern->params)key+=typeName(d.type)+";";
    if(program->compiledFunctions.contains(key)){++program->cacheHits;return program->compiledFunctions.at(key);}
    if(program->instantiations>=limits.instantiations||templateLevel>=limits.templateDepth)fail(loc,"semantic","template instantiation limit");
    Timer timer(program->templateNanoseconds);++program->instantiations;auto f=cloneFunction(pattern,map);f->templateParameters.clear();f->instantiated=true;f->id=key;
    f->result=resolve(f->result,loc);for(auto& d:f->params)d.type=resolve(d.type,loc);
    if(f->result.reference||f->result.constant)fail(loc,"semantic","qualified/reference return types unsupported");
    program->compiledFunctions[key]=f;return f;
}
Analyzer::F Analyzer::select(std::vector<F> candidates,const std::vector<E>& args,const Location& loc,const std::vector<Type>& explicitTypes,const E& object){
    struct Candidate {F f;std::vector<int> ranks;};std::vector<Candidate> viable;
    for(auto f:candidates){
        if(!f->templateParameters.empty()){
            if(explicitTypes.size()>f->templateParameters.size()||args.size()!=f->params.size())continue;
            Substitution deduced;for(size_t i=0;i<explicitTypes.size();++i)deduced[f->templateParameters[i]]=resolve(explicitTypes[i],loc);
            std::function<bool(Type,Type)> deduce=[&](Type param,Type actual){
                if(param.base==Base::Parameter){actual.reference=false;if(!param.reference||param.constant)actual.constant=false;auto it=deduced.find(param.name);if(it!=deduced.end())return it->second==actual;deduced[param.name]=actual;return true;}
                if(param.base==Base::Record&&param.name==actual.name&&param.arguments.size()==actual.arguments.size()){for(size_t i=0;i<param.arguments.size();++i)if(!deduce(param.arguments[i],actual.arguments[i]))return false;}
                return true;
            };
            bool ok=true;for(size_t i=0;i<args.size();++i){auto param=f->params[i].type;if(param.base==Base::Parameter&&deduced.contains(param.name)&&std::find(f->templateParameters.begin(),f->templateParameters.begin()+static_cast<ptrdiff_t>(explicitTypes.size()),param.name)!=f->templateParameters.begin()+static_cast<ptrdiff_t>(explicitTypes.size()))continue;ok=ok&&deduce(param,args[i]->type);}
            std::vector<Type> types;for(auto n:f->templateParameters){if(!deduced.contains(n)){ok=false;break;}types.push_back(deduced[n]);}
            if(!ok)continue;f=instantiateFunction(f,types,loc);
        }else if(!explicitTypes.empty())continue;
        if(f->params.size()!=args.size())continue;
        Candidate c{f,{}};bool ok=true;
        if(object){if(object->type.constant&&!f->constMember)continue;c.ranks.push_back(f->constMember&&!object->type.constant?1:0);}
        for(size_t i=0;i<args.size();++i){int r=rank(f->params[i].type,args[i]);if(r<0){ok=false;break;}c.ranks.push_back(r);}
        if(ok)viable.push_back(c);
    }
    if(viable.empty())fail(loc,"semantic","no viable overload (argument count, type or template deduction mismatch)");
    auto better=[&](const Candidate& a,const Candidate& b){bool strict=false;for(size_t i=0;i<a.ranks.size();++i){if(a.ranks[i]>b.ranks[i])return false;if(a.ranks[i]<b.ranks[i])strict=true;}
        if(strict)return true;
        for(size_t i=0;i<args.size();++i){auto x=a.f->params[i].type,y=b.f->params[i].type;if(x.reference&&y.reference&&same(x,y)&&x.constant!=y.constant){if(x.constant)return false;strict=true;}}
        if(strict)return true;
        return !a.f->instantiated&&b.f->instantiated;
    };
    F winner;for(auto& c:viable){bool wins=true;for(auto& other:viable)if(&other!=&c&&!better(c,other)){wins=false;break;}if(wins){winner=c.f;break;}}
    if(!winner){if(viable.size()==1)winner=viable[0].f;else fail(loc,"semantic","ambiguous overload");}
    checkAccess(winner,loc);called.insert(winner->id);
    for(size_t i=0;i<args.size();++i)if(!winner->params[i].type.reference&&winner->params[i].type.base==Base::Record&&winner->params[i].type.pointer==0)planCopy(args[i],winner->params[i].type);
    if(winner->instantiated||winner->special!=Function::Special::User){try{checkFunction(winner);}catch(const Error& error){if(winner->instantiated)fail(loc,"semantic","instantiating "+winner->id+": "+error.what());throw;}}
    return winner;
}
void Analyzer::checkAccess(const F& f,const Location& l){if(!f->publicAccess&&currentClass!=f->owner)fail(l,"semantic","private member is inaccessible");}
void Analyzer::planCopy(const E& e,Type t){
    if(t.base!=Base::Record||t.pointer>0)return;
    if(!e->lvalue&&e->type.base==Base::Record&&e->type.pointer==0&&same(e->type,t))return;
    auto f=constructor(t,{e},e->token.loc);e->copyTarget=f->id;
}
Type Analyzer::callExpression(const E& e){
    auto callee=e->args[0];std::vector<E> args(e->args.begin()+1,e->args.end());for(auto a:args)expr(a);
    std::vector<F> candidates;E object;
    if(callee->kind==Expr::Kind::Member){
        object=callee->args[0];auto t=expr(object);
        if(t.base==Base::String){
            if(callee->op=="size"||callee->op=="length"||callee->op=="empty"){
                if(!args.empty())fail(e->token.loc,"semantic","string member takes no arguments");
                e->target="native:string."+callee->op;
                return {callee->op=="empty"?Base::Bool:Base::Long};
            }
            if(callee->op=="at"){
                if(args.size()!=1||!integral(args[0]->type))fail(e->token.loc,"semantic","string::at takes 1 integer argument");
                e->target="native:string.at";
                e->lvalue=!t.constant;
                return {Base::Char};
            }
            if(callee->op=="substr"){
                if(args.empty()||args.size()>2)fail(e->token.loc,"semantic","string::substr takes 1 or 2 arguments");
                for(auto a:args)if(!integral(a->type))fail(e->token.loc,"semantic","string::substr arguments must be integral");
                e->target="native:string.substr";
                return {Base::String};
            }
            fail(callee->token.loc,"semantic","unsupported string member: "+callee->op);
        }
        if(t.base==Base::Record && (t.name=="std::vector" || t.name=="std::array")){
            Type elemTy = t.arguments[0];
            if(callee->op=="size"||callee->op=="empty"){
                if(!args.empty())fail(e->token.loc,"semantic","container method takes no arguments");
                e->target="native:vector."+callee->op;
                return {callee->op=="empty"?Base::Bool:Base::Long};
            }
            if(callee->op=="push_back"){
                if(args.size()!=1)fail(e->token.loc,"semantic","push_back requires 1 argument");
                convert(args[0],elemTy);
                e->target="native:vector.push_back";
                return {Base::Void};
            }
            if(callee->op=="pop_back"||callee->op=="clear"){
                if(!args.empty())fail(e->token.loc,"semantic","pop_back/clear takes no arguments");
                e->target="native:vector."+callee->op;
                return {Base::Void};
            }
            if(callee->op=="front"||callee->op=="back"){
                if(!args.empty())fail(e->token.loc,"semantic","front/back takes no arguments");
                e->target="native:vector."+callee->op;
                e->lvalue=true;
                return elemTy;
            }
            if(callee->op=="at"){
                if(args.size()!=1||!integral(args[0]->type))fail(e->token.loc,"semantic","at takes 1 integral argument");
                e->target="native:vector.at";
                e->lvalue=true;
                return elemTy;
            }
            if(callee->op=="begin"||callee->op=="end"){
                if(!args.empty())fail(e->token.loc,"semantic","begin/end takes no arguments");
                e->target="native:vector."+callee->op;
                Type ptrTy = elemTy;
                ptrTy.pointer = 1;
                ptrTy.constant = false;
                ptrTy.pointeeConstant = t.constant;
                return ptrTy;
            }
            if(callee->op=="reserve"){
                if(args.size()!=1||!integral(args[0]->type))fail(e->token.loc,"semantic","reserve takes 1 integral argument");
                e->target="native:vector.reserve";
                return {Base::Void};
            }
            if(callee->op=="resize"){
                if(args.empty()||args.size()>2||!integral(args[0]->type))fail(e->token.loc,"semantic","resize takes 1 or 2 arguments");
                if(args.size()==2)convert(args[1],elemTy);
                e->target="native:vector.resize";
                return {Base::Void};
            }
            fail(callee->token.loc,"semantic","unsupported container method: "+callee->op);
        }
        auto c=record(t,e->token.loc);for(auto f:c->methods)if(!f->constructor&&!f->destructor&&f->name.text==callee->op)candidates.push_back(program->compiledFunctions.at(f->id));
    }else if(callee->kind==Expr::Kind::Name){
        std::string name = callee->op;
        if(name=="std::make_pair"||name=="make_pair"){
            if(args.size()!=2)fail(e->token.loc,"semantic","make_pair requires 2 arguments");
            Type pairTy{Base::Record};
            pairTy.name="std::pair";
            pairTy.arguments={args[0]->type,args[1]->type};
            e->target="native:make_pair";
            return pairTy;
        }
        if(name=="std::move"||name=="move"){
            if(args.size()!=1)fail(e->token.loc,"semantic","move requires 1 argument");
            e->target="native:move";
            return args[0]->type;
        }
        if(name=="std::swap"||name=="swap"){
            if(args.size()!=2)fail(e->token.loc,"semantic","swap requires 2 arguments");
            modifiable(args[0]);modifiable(args[1]);
            if(args[0]->type!=args[1]->type)fail(e->token.loc,"semantic","swap arguments type mismatch");
            e->target="native:swap";
            return {Base::Void};
        }
        if(name=="std::min"||name=="min"||name=="std::max"||name=="max"){
            if(args.size()!=2)fail(e->token.loc,"semantic","min/max requires 2 arguments");
            if(!numeric(args[0]->type)||!numeric(args[1]->type))fail(e->token.loc,"semantic","min/max requires arithmetic arguments");
            e->target=std::string("native:")+(name.ends_with("min")?"min":"max");
            return common(args[0]->type,args[1]->type);
        }
        if(name=="std::sort"||name=="sort"||name=="std::reverse"||name=="reverse"){
            if(args.size()!=2)fail(e->token.loc,"semantic","sort/reverse requires 2 iterators");
            if(args[0]->type.pointer<=0||args[1]->type.pointer<=0)fail(e->token.loc,"semantic","sort/reverse arguments must be iterators/pointers");
            e->target=std::string("native:")+(name.ends_with("sort")?"sort":"reverse");
            return {Base::Void};
        }
        if(name=="std::find"||name=="find"){
            if(args.size()!=3)fail(e->token.loc,"semantic","find requires 3 arguments (first, last, val)");
            if(args[0]->type.pointer<=0||args[1]->type.pointer<=0)fail(e->token.loc,"semantic","find first/last must be iterators/pointers");
            e->target="native:find";
            return args[0]->type;
        }
        if(name=="std::count"||name=="count"){
            if(args.size()!=3)fail(e->token.loc,"semantic","count requires 3 arguments (first, last, val)");
            if(args[0]->type.pointer<=0||args[1]->type.pointer<=0)fail(e->token.loc,"semantic","count first/last must be iterators/pointers");
            e->target="native:count";
            return {Base::Long};
        }
        Type constructed=callee->type;
        if(classPatterns.contains(callee->op)||program->compiledClasses.contains(callee->op)||(callee->op.starts_with("std::")&&(callee->op.ends_with("vector")||callee->op.ends_with("pair")))){
            constructed.base=Base::Record;constructed.name=callee->op;constructed.arguments=callee->typeArguments;if(callee->op==currentClass)constructed=program->compiledClasses.at(currentClass)->type;
        }
        if(constructed.base!=Base::Void){
            constructed=resolve(constructed,e->token.loc);
            if(constructed.name=="std::pair"){
                e->kind=Expr::Kind::Construct;
                e->args=args;
                e->target="native:pair.$ctor";
                return constructed;
            }
            if(constructed.name=="std::vector"){
                e->kind=Expr::Kind::Construct;
                e->args=args;
                e->target="native:vector.$ctor";
                return constructed;
            }
            if(constructed.base!=Base::Record)fail(e->token.loc,"semantic","dependent scalar functional casts unsupported");
            auto f=constructor(constructed,args,e->token.loc);
            e->kind=Expr::Kind::Construct;
            e->args=args;
            e->target=f->id;
            checkDestructor(constructed,e->token.loc);
            return constructed;
        }
        bool memberName=false;if(!currentClass.empty()){auto c=program->compiledClasses.at(currentClass);for(auto f:c->methods)if(!f->constructor&&!f->destructor&&f->name.text==callee->op){candidates.push_back(program->compiledFunctions.at(f->id));memberName=true;}}
        for(size_t i=scopes.size();i>1;--i)if(scopes[i-1].contains(callee->op))fail(callee->token.loc,"semantic","called name denotes an object");
        if(memberName){object=std::make_shared<Expr>(Expr::Kind::Name,callee->token);object->type=program->compiledClasses.at(currentClass)->type;object->type.constant=constMethod;e->implicitMember=true;}
        else {if(scopes[0].contains(callee->op))fail(callee->token.loc,"semantic","called name denotes an object");candidates=functions[callee->op];auto ts=templates[callee->op];candidates.insert(candidates.end(),ts.begin(),ts.end());}
        if(callee->op=="main")fail(callee->token.loc,"semantic","calling main is unsupported");
    }else fail(e->token.loc,"semantic","unsupported call target");
    auto f=select(candidates,args,e->token.loc,callee->typeArguments,object);e->target=f->id;return f->result;
}
}
