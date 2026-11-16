#include "praxis/runtime/interpreter.hpp"
#include <algorithm>
#include <exception>
#include "praxis/common/timing.hpp"
namespace praxis {
Interpreter::O Interpreter::object(const Evaluation& e,const Location& l){
    auto o=e.object;if(!o&&e.value.type.base==Base::Record)o=std::get<O>(e.value.data);
    if(!o)fail(l,"runtime","expression has no object identity");requireAlive(o,l);return o;
}
void Interpreter::requireAlive(const O& o,const Location& l){if(!o||!o->alive)fail(l,"runtime","object lifetime has ended");}
void Interpreter::detachTemporary(const O& o){auto it=std::find(temporaries.begin(),temporaries.end(),o);if(it!=temporaries.end())temporaries.erase(it);}
void Interpreter::cleanup(Scope& scope){auto owned=std::move(scope.owned);scope.owned.clear();std::exception_ptr first;
    for(auto it=owned.rbegin();it!=owned.rend();++it)try{destroy(*it);}catch(...){if(!first)first=std::current_exception();}
    scope.names.clear();if(first)std::rethrow_exception(first);
}
void Interpreter::cleanupTemporaries(size_t mark){std::exception_ptr first;while(temporaries.size()>mark){auto o=temporaries.back();temporaries.pop_back();try{destroy(o);}catch(...){if(!first)first=std::current_exception();}}if(first)std::rethrow_exception(first);}
void Interpreter::destroy(const O& o){
    if(!o||!o->alive||o->destroying)return;
    ++cleanupDepth;struct CleanupExit{int& depth;~CleanupExit(){--depth;}}exit{cleanupDepth};
    Timer timer(profile.destructorNanoseconds);o->destroying=true;std::exception_ptr first;
    if(o->type.base==Base::Record&&o->complete){++profile.destructors;
        if(o->definition&&!o->definition->destructor.empty())try{call(functions.at(o->definition->destructor),{},o);}catch(...){first=std::current_exception();}
    }
    for(auto it=o->members.rbegin();it!=o->members.rend();++it)try{destroy(*it);}catch(...){if(!first)first=std::current_exception();}
    for(auto it=o->elements.rbegin();it!=o->elements.rend();++it)try{destroy(*it);}catch(...){if(!first)first=std::current_exception();}
    o->generation++;
    o->complete=false;o->alive=false;o->initialized=false;o->destroying=false;if(liveObjects)--liveObjects;
    if(first)std::rethrow_exception(first);
}
void Interpreter::discardReturn(Flow& flow){if(flow.kind==Flow::Return&&flow.value.type.base==Base::Record&&std::holds_alternative<O>(flow.value.data)){auto o=std::get<O>(flow.value.data);flow.value=zero({Base::Void});destroy(o);}}
void Interpreter::finish(){std::exception_ptr first;try{cleanupTemporaries(0);}catch(...){first=std::current_exception();}try{cleanup(globals);}catch(...){if(!first)first=std::current_exception();}if(first)std::rethrow_exception(first);}
void Interpreter::construct(const O& o,const std::string& id,const std::vector<Evaluation>& args){
    Timer timer(profile.constructorNanoseconds);try{call(functions.at(id),args,o);o->initialized=true;o->complete=true;++profile.constructions;}
    catch(...){auto primary=std::current_exception();try{destroy(o);}catch(...){}std::rethrow_exception(primary);}
}
Interpreter::O Interpreter::copyObject(const O& source,Type t,const std::string& id,const Location& l){
    requireAlive(source,l);if(!source->initialized)fail(l,"runtime","copy of incompletely initialized object");
    if (source->type.base == Base::Record && (source->type.name == "std::vector" || source->type.name == "std::array" || source->type.name == "std::pair")) {
        ++profile.containerOperations;
        auto result = allocate(t, true);
        result->location = l;
        result->initialized = true;
        result->complete = true;
        for (auto e : source->elements) {
            O copy;
            if (e->type.base == Base::Record && e->type.pointer == 0) copy = copyObject(e, e->type, "", l);
            else {
                copy = allocate(e->type, true);
                copy->value = e->value;
            }
            result->elements.push_back(copy);
        }
        return result;
    }
    auto result=allocate(t,false);result->location=l;auto ctor=id.empty()?result->definition->copyConstructor:id;
    construct(result,ctor,{{{},source}});++profile.copies;return result;
}
void Interpreter::assignObject(const O& destination,const O& source,const Location& l){
    requireAlive(destination,l);requireAlive(source,l);if(destination==source)return;
    if(!destination->complete||!source->complete)fail(l,"runtime","assignment involving incomplete object");
    if(destination->type.base==Base::Record && (destination->type.name == "std::vector" || destination->type.name == "std::array" || destination->type.name == "std::pair")) {
        ++profile.containerOperations;
        for (auto it = destination->elements.rbegin(); it != destination->elements.rend(); ++it)
            destroy(*it);
        destination->elements.clear();
        destination->initialized = true;
        destination->complete = true;
        for (auto e : source->elements) {
            O copy;
            if (e->type.base == Base::Record && e->type.pointer == 0) copy = copyObject(e, e->type, "", l);
            else {
                copy = allocate(e->type, true);
                copy->value = e->value;
            }
            destination->elements.push_back(copy);
        }
        return;
    }
    if(destination->definition->key!=source->definition->key)fail(l,"runtime","class assignment type mismatch");
    ++profile.assignments;
    for(auto& d:destination->definition->fields){auto a=destination->fields.at(d.name.text),b=source->fields.at(d.name.text);
        if(d.type.base==Base::Record && d.type.pointer==0)assignObject(a,b,l);else{a->value=read({{},b},l);a->initialized=true;}}
}
void Interpreter::initializeMembers(const O& o,const std::shared_ptr<Function>& f,const std::vector<Evaluation>& args){
    if(f->special==Function::Special::CopyConstructor){auto source=object(args.at(0),f->name.loc);
        for(auto& d:o->definition->fields){O member;auto from=source->fields.at(d.name.text);
            if(d.type.base==Base::Record && d.type.pointer==0)member=copyObject(from,d.type,"",d.name.loc);
            else{auto v=read({{},from},d.name.loc);member=allocate(d.type,false);member->value=v;member->initialized=true;}
            o->fields[d.name.text]=member;o->members.push_back(member);
        }
        o->initialized=true;return;
    }
    for(size_t i=0;i<o->definition->fields.size();++i){auto& d=o->definition->fields[i];auto& init=f->initializers.at(i);size_t mark=temporaries.size();auto member=allocate(d.type,false);member->location=d.name.loc;member->zeroInitialize=o->zeroInitialize;if(o->zeroInitialize&&(d.type.base!=Base::Record || d.type.pointer>0))member->initialized=true;o->fields[d.name.text]=member;
        try{
            if(d.type.base==Base::Record && d.type.pointer==0){
                if(init.constructor=="$container"){
                    member->complete=true;
                    member->initialized=true;
                    if(!init.args.empty() && init.args.size()==1){
                        auto a=eval(init.args[0]);
                        auto src=a.object ? a.object : (std::holds_alternative<O>(a.value.data)?std::get<O>(a.value.data):nullptr);
                        if(src){
                            destroy(member);
                            member=copyObject(src,d.type,"",d.name.loc);
                            o->fields[d.name.text]=member;
                        }
                    }
                }else{
                    std::vector<Evaluation> values;for(auto e:init.args)values.push_back(eval(e));
                    if(init.constructor=="$elide"){auto source=object(values.at(0),d.name.loc);destroy(member);member=source;member->type=d.type;detachTemporary(member);o->fields[d.name.text]=member;}
                    else {if(init.valueInitialize&&functions.at(init.constructor)->special==Function::Special::DefaultConstructor)member->zeroInitialize=true;construct(member,init.constructor,values);}
                }
            }
            else if(!init.args.empty()){member->value=convertValue(value(init.args[0]),d.type,d.name.loc);member->initialized=true;}
            else if(init.constructor=="$zero"){member->value=zero(d.type);member->initialized=true;}
            o->members.push_back(member);cleanupTemporaries(mark);
        }catch(...){auto error=std::current_exception();try{destroy(member);cleanupTemporaries(mark);}catch(...){}std::rethrow_exception(error);}
    }
    o->initialized=true;
}
bool Interpreter::test(const E& e){size_t mark=temporaries.size();try{bool b=truth(value(e));cleanupTemporaries(mark);return b;}catch(...){auto primary=std::current_exception();try{cleanupTemporaries(mark);}catch(...){}std::rethrow_exception(primary);}}
void Interpreter::discard(const E& e){size_t mark=temporaries.size();try{eval(e);cleanupTemporaries(mark);}catch(...){auto primary=std::current_exception();try{cleanupTemporaries(mark);}catch(...){}std::rethrow_exception(primary);}}
}
