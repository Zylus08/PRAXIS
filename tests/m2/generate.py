from pathlib import Path
import json
root=Path(__file__).resolve().parents[2];cases=[]
def case(n,c,s,o='',p=None):cases.append(dict(name=n,category=c,source='#include <iostream>\n#include <string>\n'+s,stdout=o,phase=p,exit=1 if p else 0))
case('basic_class','class','class C {int n;public:C(int x):n(x){}void add(int x){n+=x;}int get() const{return n;}};int main(){C c(2);c.add(3);std::cout<<c.get();}','5')
case('struct','class','struct S {int x;int y;};int main(){S s;s.x=3;s.y=4;std::cout<<s.x+s.y;}','7')
case('constructor_destructor','lifetime','struct G {int n;G(int x):n(x){std::cout<<"C"<<n;}~G(){std::cout<<"D"<<n;}};int main(){G a(1);{G b(2);}std::cout<<"X";}','C1C2D2XD1')
case('member_initializer_order','construction','struct G {int n;G(int x):n(x){std::cout<<n;}~G(){std::cout<<n;}};struct S {G a;G b;S():b(2),a(1){std::cout<<"P";}~S(){std::cout<<"Q";}};int main(){S s;}','12PQ21')
case('copy_constructor','copy','class S {int x;public:S(int n):x(n){}S(const S& s):x(s.x){std::cout<<"C";}int get() const{return x;}};int main(){S a(7);S b=a;std::cout<<b.get();}','C7')
case('copy_assignment','copy','struct S {int x;S(int n):x(n){}S(const S& s):x(s.x){std::cout<<"C";}};int main(){S a(1);S b=a;b.x=2;a=b;std::cout<<a.x<<b.x;}','C22')
case('const_member','const','struct S {int x;S(int n):x(n){}int get() const{return x;}};int main(){const S s(8);std::cout<<s.get();}','8')
case('access_control','access','class S {int x;int hidden() const{return x;}public:S(int n):x(n){}int get() const{return hidden();}};int main(){S s(9);std::cout<<s.get();}','9')
case('free_overload','overload','int f(int x){return 1;}int f(double x){return 2;}int main(){std::cout<<f(1)<<f(1.0)<<f(\'a\');}','121')
case('constructor_overload','overload','struct S {int x;S(int n):x(1){}S(double n):x(2){}};int main(){S a(2);S b(2.0);std::cout<<a.x<<b.x;}','12')
case('raii_return','lifetime','struct G {int n;G(int x):n(x){std::cout<<"C"<<n;}~G(){std::cout<<"D"<<n;}};int f(){G a(1);{G b(2);return 7;}}int main(){std::cout<<f();}','C1C2D2D17')
case('raii_loop_control','lifetime','struct G {int n;G(int x):n(x){std::cout<<"C"<<n;}~G(){std::cout<<"D"<<n;}};int main(){for(int i=0;i<3;++i){G g(i);if(i==0)continue;if(i==1)break;}}','C0D0C1D1')
case('nested_destruction','lifetime','struct A {A(){std::cout<<"a";}~A(){std::cout<<"A";}};struct B {A a;B(){std::cout<<"b";}~B(){std::cout<<"B";}};int main(){B b;}','abBA')
case('function_template','template','template<typename T>T add(T a,T b){return a+b;}int main(){std::cout<<add<int>(2,3)<<" "<<add<double>(1.5,2.0);}','5 3.5')
case('class_template','template','template<typename T>class Box {T value;public:Box(T x):value(x){}T get() const{return value;}};int main(){Box<int> a(3);Box<double> b(2.5);std::cout<<a.get()<<" "<<b.get();}','3 2.5')
case('template_deduction','template','template<class T>T add(T a,T b){return a+b;}int main(){std::cout<<add(3,4)<<" "<<add(1.25,2.5);}','7 3.75')
case('ambiguous_overload','rejection','int f(long long x){return 1;}int f(double x){return 2;}int main(){f(1);}',p='semantic')
case('invalid_template','rejection','template<class T>T add(T a,T b){return a+b;}struct S {};int main(){S a;add(a,a);}',p='semantic')
more=[
('default_member_initializer','construction','struct S {int x=7;};int main(){S a;std::cout<<a.x;}','7'),
('scalar_value_init_member','construction','struct S {int x;S():x(){}};int main(){S a;std::cout<<a.x;}','0'),
('copy_nested','copy','struct A {int x;A(int n):x(n){}A(const A& a):x(a.x){std::cout<<"C";}};struct B {A a;B(int n):a(n){}};int main(){B a(3);B b=a;b.a.x=9;std::cout<<a.a.x<<b.a.x;}','C39'),
('assign_nested','copy','struct A {int x;A(int n):x(n){}};struct B {A a;B(int n):a(n){}};int main(){B a(3);B b(4);a=b;b.a.x=9;std::cout<<a.a.x<<b.a.x;}','49'),
('object_reference','reference','struct S {int x;S(int n):x(n){}};void set(S& s){s.x=8;}int main(){S a(1);S& b=a;set(b);std::cout<<a.x;}','8'),
('const_object_reference','reference','struct S {int x;S(int n):x(n){}};int get(const S& s){return s.x;}int main(){S a(7);std::cout<<get(a);}','7'),
('class_by_value','copy','struct S {int x;S(int n):x(n){}S(const S& s):x(s.x){std::cout<<"C";}~S(){std::cout<<"D"<<x;}};void f(S s){s.x=8;}int main(){S s(1);f(s);std::cout<<s.x;}','CD81D1'),
('return_local_object','copy','struct S {int x;S(int n):x(n){std::cout<<"N";}S(const S& s):x(s.x){std::cout<<"C";}~S(){std::cout<<"D";}};S f(){S s(7);return s;}int main(){S a=f();std::cout<<a.x;}','NCD7D'),
('return_prvalue_object','temporary','struct S {int x;S(int n):x(n){std::cout<<"N";}S(const S& s):x(s.x){std::cout<<"C";}~S(){std::cout<<"D";}};S f(){return S(7);}int main(){S a=f();std::cout<<a.x;}','N7D'),
('temporary_statement','temporary','struct G {G(int n){std::cout<<n;}~G(){std::cout<<"D";}};int main(){G(3);std::cout<<"X";}','3DX'),
('temporary_method','temporary','struct S {S(){std::cout<<"C";}~S(){std::cout<<"D";}int get() const{return 7;}};int main(){std::cout<<S().get();std::cout<<"X";}','C7DX'),
('temporary_condition','temporary','struct S {S(){std::cout<<"C";}~S(){std::cout<<"D";}bool test() const{return true;}};int main(){if(S().test())std::cout<<"B";}','CDB'),
('prvalue_parameter','temporary','struct S {S(int n){std::cout<<n;}S(const S& s){std::cout<<"C";}~S(){std::cout<<"D";}};void f(S s){std::cout<<"F";}int main(){f(S(2));std::cout<<"X";}','2FDX'),
('const_overload','overload','struct S {int f(){return 1;}int f() const{return 2;}};int main(){S s;const S c;std::cout<<s.f()<<c.f();}','12'),
('member_overload','overload','struct S {int f(int x){return 1;}int f(double x){return 2;}};int main(){S s;std::cout<<s.f(2)<<s.f(2.0);}','12'),
('reference_overload','overload','int f(int& x){return 1;}int f(const int& x){return 2;}int main(){int x=1;const int y=1;std::cout<<f(x)<<f(y);}','12'),
('template_nontemplate_tie','overload','template<class T>int f(T x){return 1;}int f(int x){return 2;}int main(){std::cout<<f(2)<<f(2.0)<<f<int>(2);}','211'),
('out_of_class_member','class','class S {int x;public:S(int n):x(n){}int get() const;};int S::get() const{return x;}int main(){S s(6);std::cout<<s.get();}','6'),
('member_shadowing','scope','int x=9;struct S {int x;S(int n):x(n){}int f(int x){return x;}int g(){return x;}};int main(){S s(3);std::cout<<s.f(2)<<s.g()<<x;}','239'),
('implicit_member_call','class','struct S {int x=3;int a() const{return x;}int b() const{return a()+1;}};int main(){S s;std::cout<<s.b();}','4'),
('global_destruction','lifetime','struct G {int n;G(int v):n(v){std::cout<<"C"<<n;}~G(){std::cout<<"D"<<n;}};G a(1);G b(2);int main(){std::cout<<"M";}','C1C2MD2D1'),
('recursive_raii','lifetime','struct G {int n;G(int v):n(v){std::cout<<n;}~G(){std::cout<<n;}};int f(int n){G g(n);if(n==0)return 0;return f(n-1);}int main(){f(3);}','32100123'),
('nested_template','template','template<class T>struct Box {T value;Box(T x):value(x){}T get() const{return value;}};int main(){Box<int> inner(3);Box<Box<int>> outer(inner);std::cout<<outer.value.value;}','3'),
('template_reference','template','template<class T>void set(T& x,T v){x=v;}int main(){int x=1;set(x,9);std::cout<<x;}','9'),
('template_two_parameters','template','template<class T,class U>T choose(T a,U b){return a;}int main(){std::cout<<choose(7,2.5)<<choose<double,int>(1.5,3);}','71.5'),
('template_class_copy','template','template<class T>struct Box {T x;Box(T v):x(v){}};int main(){Box<int> a(4);Box<int> b=a;b.x=7;std::cout<<a.x<<b.x;}','47'),
('template_class_dtor','template','template<class T>struct Box {T x;Box(T v):x(v){}~Box(){std::cout<<x;}};int main(){Box<int> a(4);Box<double> b(2.5);}','2.54'),
('template_repeated','template','template<class T>T twice(T a){return a+a;}int main(){std::cout<<twice(2)<<twice(3)<<twice(1.5);}','463'),
('class_string_member','native','class S {std::string s;public:S(std::string v):s(v){}std::string get() const{return s;}};int main(){S s("hi");std::cout<<s.get();}','hi'),
('constructor_body_mutation','construction','struct S {int x;S(int n){x=n+1;}};int main(){S s(4);std::cout<<s.x;}','5')]
for args in more:case(*args)
invalid=[
('default_ctor_suppressed','struct S {S(int n){}};int main(){S s;}'),('private_field','class S {int x;};int main(){S s;s.x=1;}'),('private_method','class S {int f(){return 1;}};int main(){S s;s.f();}'),('private_ctor','class S {S(){}};int main(){S s;}'),('private_dtor','class S {~S(){}public:S(){}};int main(){S s;}'),
('const_field_mutation','struct S {int x;S():x(1){}void f() const{x=2;}};int main(){}'),('const_object_mutation','struct S {int x;S():x(1){}};int main(){const S s;s.x=2;}'),('const_method_call','struct S {void f(){}};int main(){const S s;s.f();}'),('const_nested_member','struct A {int x;A():x(1){}};struct B {A a;};int main(){const B b;b.a.x=2;}'),
('invalid_member','struct S {};int main(){S s;s.missing=1;}'),('class_type_mismatch','struct A {};struct B {};int main(){A a;B b;a=b;}'),('class_reference_mismatch','struct A {};struct B {};int main(){A a;B& b=a;}'),('temporary_reference','struct S {};int main(){const S& s=S();}'),
('duplicate_initializer','struct S {int x;S():x(1),x(2){}};int main(){}'),('unknown_initializer','struct S {S():missing(1){}};int main(){}'),('recursive_member','struct S {S value;};int main(){}'),('deleted_assignment','struct S {const int x;S():x(1){}};int main(){S a;S b;a=b;}'),
('ambiguous_cross_rank','int f(int a,double b){return 1;}int f(double a,int b){return 2;}int main(){f(1,1);}'),('ambiguous_ref_value','int f(int a){return 1;}int f(int& a){return 2;}int main(){int x=1;f(x);}'),
('template_wrong_count','template<class T>T f(T x){return x;}int main(){f<int,double>(1);}'),('template_conflicting_deduction','template<class T>T f(T x,T y){return x;}int main(){f(1,2.0);}'),('class_template_wrong_count','template<class T>struct S {};int main(){S<int,double> s;}'),('template_invalid_member','template<class T>int f(T x){return x.missing;}int main(){f(1);}'),('class_template_invalid_body','template<class T>struct S {T x;int f(){return x.bad;}};int main(){S<int> s;s.f();}')]
for n,s in invalid:case(n,'rejection',s,p='semantic')
for n,s in [('inheritance','struct A {};struct B:A {};int main(){}'),('virtual','struct S {virtual void f(){}};int main(){}'),('throw','int main(){throw 1;}'),('try_catch','int main(){try{}catch(int x){}}'),('pointer','struct S {};int main(){S* p;}'),('user_assignment','struct S {S& operator=(const S& x){return x;}};int main(){}')]:case(n,'unsupported',s,p='parse')
case('constructor_failure','runtime','struct A {A(){std::cout<<"A";}~A(){std::cout<<"a";}};struct B {A a;B(){int z=0;int x=1/z;}~B(){std::cout<<"b";}};int main(){B b;}','Aa','runtime')
case('partial_member_failure','runtime','struct A {A(){std::cout<<"A";}~A(){std::cout<<"a";}};struct B {B(){int z=0;int x=1/z;}~B(){std::cout<<"b";}};struct C {A a;B b;~C(){std::cout<<"c";}};int main(){C c;}','Aa','runtime')
case('uninitialized_member','runtime','struct S {int x;};int main(){S s;std::cout<<s.x;}','','runtime')
case('later_member_read','runtime','struct S {int a;int b;S():a(b),b(2){}};int main(){S s;}','','runtime')
case('runtime_unwind','runtime','struct G {int n;G(int v):n(v){}~G(){std::cout<<n;}};int f(){G a(2);int z=0;return 1/z;}int main(){G a(1);f();}','21','runtime')
for depth in range(1,7):
 s='struct L0 {L0(){std::cout<<"0";}~L0(){std::cout<<"0";}};'
 for i in range(1,depth+1):s+=f'struct L{i} {{L{i-1} inner;L{i}(){{std::cout<<"{i}";}}~L{i}(){{std::cout<<"{i}";}}}};'
 case(f'nesting_{depth}','generated_lifetime',s+f'int main(){{L{depth} value;}}',''.join(map(str,range(depth+1)))+''.join(map(str,range(depth,-1,-1))))
for n in [1,2,7,31,100,1000]:case(f'objects_{n}','stress',f'int created=0;int destroyed=0;struct G {{G(){{++created;}}~G(){{++destroyed;}}}};int main(){{for(int i=0;i<{n};++i){{G g;}}std::cout<<created<<":"<<destroyed;}}',f'{n}:{n}')
for ty,v,o in [('int','3','6'),('long long','3000000000LL','6000000000'),('float','1.25f','2.5'),('double','2.75','5.5'),('std::string','"ab"','abab')]:case('deduce_'+ty.replace(' ','_').replace('::','_'),'generated_template',f'template<typename T>T twice(T x){{return x+x;}}int main(){{std::cout<<twice({v});}}',o)
for c in cases:
 if c['name']=='deduce_std_string':c['source']='#include <iostream>\n#include <string>\ntemplate<typename T>T twice(T x){return x+x;}int main(){std::string s="ab";std::cout<<twice(s);}'
case('unused_template_member','template','template<class T>struct S {T x;int f(){return x.bad;}};int main(){S<int> s;s.x=3;std::cout<<s.x;}','3')
case('partial_explicit_conflict','rejection','template<class T,class U>U f(T a,U b,U c){return b;}int main(){f<int>(1,2,3.0);}',p='semantic')

case('value_initialized_temporary','construction','struct S {int x;};int main(){std::cout<<S().x;}','0')
case('value_initialized_member','construction','struct A {int x;};struct B {A a;B():a(){}};int main(){B b;std::cout<<b.a.x;}','0')
case('global_zero_class','construction','struct S {int x;S(){std::cout<<x;}};S s;int main(){std::cout<<s.x;}','00')
case('direct_prvalue_elision','temporary','struct S {S(int n){std::cout<<n;}S(const S& s){std::cout<<"C";}~S(){std::cout<<"D";}};int main(){S s(S(3));}','3D')
case('member_prvalue_elision','temporary','struct S {S(int n){std::cout<<n;}S(const S& s){std::cout<<"C";}~S(){std::cout<<"D";}};struct B {S s;B():s(S(3)){}~B(){std::cout<<"B";}};int main(){B b;}','3BD')
case('self_assignment','copy','struct S {int x;S(int n):x(n){}S(const S& s):x(s.x){std::cout<<"C";}};int main(){S s(4);s=s;std::cout<<s.x;}','4')
case('template_const_deduction','template','template<class T>void show(T& x){std::cout<<x;}int main(){const int n=4;show(n);}','4')
case('nested_type_deduction','template','template<class T>struct Box {T value;Box(T x):value(x){}};template<class T>T get(const Box<T>& box){return box.value;}int main(){Box<int> b(8);std::cout<<get(b);}','8')
case('unselected_template_body','template','template<class T>int f(T x){return x.missing;}int f(int x){return 7;}int main(){std::cout<<f(1);}','7')
case('duplicate_template','rejection','template<class T>T f(T x){return x;}template<class U>U f(U x){return x;}int main(){}',p='semantic')
case('ctor_return_type','rejection','struct S {int S(){return 1;}};int main(){}',p='semantic')
case('const_nested_assignment','rejection','struct A {const int x;A():x(1){}};struct B {A a;};int main(){B a;B b;a=b;}',p='semantic')
case('private_copy','rejection','class S {S(const S& s){}public:S(){}};int main(){S a;S b=a;}',p='semantic')
case('missing_method_definition','rejection','struct S {int f();};int main(){S s;s.f();}',p='semantic')
case('destructor_local_object','lifetime','struct A {~A(){std::cout<<"A";}};struct B {~B(){A a;std::cout<<"B";}};int main(){B b;}','BA')
case('while_cleanup','lifetime','struct G {int n;G(int x):n(x){}~G(){std::cout<<n;}};int main(){int i=0;while(i<3){G g(i);++i;if(i<2)continue;break;}}','01')
case('do_cleanup','lifetime','struct G {int n;G(int x):n(x){}~G(){std::cout<<n;}};int main(){int i=0;do{G g(i);++i;continue;}while(i<3);}','012')
case('destructor_failure_cleanup','runtime','struct A {~A(){std::cout<<"A";}};struct B {A a;~B(){std::cout<<"B";int z=0;int x=1/z;}};int main(){A first;B second;}','BAA','runtime')
case('parameter_constructor_failure','runtime','struct G {G(){std::cout<<"G";}~G(){std::cout<<"g";}};struct B {B(G g){int z=0;int x=1/z;}~B(){std::cout<<"b";}};int main(){G g;B b(g);}','Ggg','runtime')

case('most_vexing_empty','unsupported','struct S {};int main(){S s();}',p='parse')
case('most_vexing_nested','unsupported','struct S {};int main(){S s(S());}',p='parse')
case('parenthesized_prvalue','temporary','struct S {S(){std::cout<<"C";}~S(){std::cout<<"D";}};int main(){S s((S()));}','CD')
case('user_ctor_not_zeroed','runtime','struct S {int x;S(){}};int main(){std::cout<<S().x;}','','runtime')
case('nested_value_zero','construction','struct A {int x;A(){}};struct B {A a;};int main(){B b=B();std::cout<<b.a.x;}','0')
path=root/'tests/m2/fixtures';path.mkdir(parents=True,exist_ok=True)
for c in cases:(path/(c['name']+'.cpp')).write_text(c['source'])
(root/'tests/m2/cases.json').write_text(json.dumps(cases,indent=2))
for c in cases[:18]:(root/'examples/m2'/(c['name']+'.cpp')).write_text(c['source'])
print(len(cases),'fixtures;',sum(c['phase'] is None for c in cases),'reference candidates')
