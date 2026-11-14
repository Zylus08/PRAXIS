#include "praxis/frontend/parser.hpp"
#include "praxis/frontend/preprocessor.hpp"
#include "praxis/semantics/analyzer.hpp"
#include "praxis/runtime/interpreter.hpp"
#include <iostream>
#include <sstream>
using namespace praxis;
Program parse(const std::string& text,bool script=false){Preprocessor pp;auto p=Parser(pp.run({"unit-m2.cpp",text})).parse(script);p.iostream=pp.iostream;p.strings=pp.string;return p;}
int main(){int count=0;auto check=[&](bool value,const char* name){++count;if(!value)throw std::runtime_error(name);};
 try{
    auto p=parse("class C {int x;public:C(int n):x(n){}int get()const{return x;}};struct S {int x;};int main(){C c(3);return c.get();}");
    check(p.items[0].record!=nullptr,"class AST");check(!p.items[0].record->fields[0].publicAccess,"class private default");check(p.items[1].record->fields[0].publicAccess,"struct public default");check(p.items[0].record->methods[0]->constructor,"constructor AST");check(p.items[0].record->methods[1]->constMember,"const method AST");
    Analyzer{}.check(p);check(p.compiledClasses.size()==2,"class registry");check(!p.compiledClasses.at("C")->copyConstructor.empty(),"implicit copy availability");std::ostringstream out;Interpreter r(out);check(r.run(p)==3,"member dispatch return");check(r.statistics().constructions==1,"class construction count");check(r.statistics().destructors==1,"class destruction count");
    auto templates=parse("template<class T>T twice(T x){return x+x;}template<class T>struct Box {T value;Box(T x):value(x){}};int main(){Box<int> a(1);Box<double> b(2.0);twice(1);twice(2);twice(1.5);}");
    check(templates.items[0].function->params[0].type.base==Base::Parameter,"dependent function parameter retained");Analyzer{}.check(templates);
    check(templates.instantiations==4,"separate function/class instantiations");check(templates.cacheHits>0,"instantiation cache reuse");check(templates.compiledClasses.contains("Box<int>")&&templates.compiledClasses.contains("Box<double>"),"class specialization identities");
    check(templates.items[0].function->params[0].type.base==Base::Parameter,"template pattern immutable");check(templates.compiledClasses.at("Box<int>")->fields[0].type.base==Base::Int,"int member substitution");check(templates.compiledClasses.at("Box<double>")->fields[0].type.base==Base::Double,"double member substitution");
    auto life=parse("#include <iostream>\nstruct G {G(){std::cout<<\"C\";}~G(){std::cout<<\"D\";}};G g;",true);Analyzer{}.check(life);std::ostringstream trace;Interpreter persistent(trace);persistent.run(life);check(trace.str()=="C","persistent static lifetime");persistent.finish();check(trace.str()=="CD","explicit session cleanup");persistent.finish();check(trace.str()=="CD"&&persistent.statistics().destructors==1,"cleanup idempotence");
    auto object=std::make_shared<Object>();object->initialized=true;object->type={Base::Int};object->value={{Base::Int},std::int64_t{7}};
    {Scope alias;alias.names["r"]=object;}check(object->alive,"reference scope does not own referent");
    {Scope owner;owner.owned.push_back(object);}check(!object->alive,"storage expiration tombstone");bool rejected=false;try{read({{},object},{"unit"});}catch(const Error& e){rejected=e.phase=="runtime";}check(rejected,"expired reference read diagnosed");
    auto overload=parse("int f(int x){return 1;}int f(double x){return 2;}int main(){return f(1.0);}");Analyzer{}.check(overload);auto expression=overload.items.back().function->body->children[0]->expr;check(expression->target=="f(double;)","exact overload selected in AST");

    Limits small;small.objects=3;auto quota=parse("int main(){int a[5];}");Analyzer{}.check(quota);std::ostringstream sink;Interpreter limited(sink,small);rejected=false;try{limited.run(quota);}catch(const Error& e){rejected=e.phase=="runtime";}
    check(rejected,"object quota diagnosed");check(limited.liveObjectCount()==0,"partial array allocation cleaned");
    auto recover=parse("int main(){return 2;}");Analyzer{}.check(recover);check(limited.run(recover)==2,"interpreter reusable after quota failure");
    auto badCtor=parse("struct A {A(){}};struct B {A a;B(){int z=0;int x=1/z;}};int main(){B b;}");Analyzer{}.check(badCtor);Interpreter failing(sink);rejected=false;try{failing.run(badCtor);}catch(const Error&){rejected=true;}
    check(rejected,"constructor failure propagated");check(failing.liveObjectCount()==0,"failed construction leaves no live objects");check(failing.statistics().destructors==1,"only complete member destroyed");
    auto badConversion=parse("void f(char x){}int main(){f(200);}");Analyzer{}.check(badConversion);Interpreter conversions(sink);try{conversions.run(badConversion);}catch(const Error&){}
    check(conversions.liveObjectCount()==0,"failed parameter conversion leaves no live objects");
    std::cout<<count<<" M2 unit checks passed\n";return 0;
 }catch(const Error& e){std::cerr<<diagnostic(e)<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
