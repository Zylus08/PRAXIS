from pathlib import Path
import json

root = Path(__file__).resolve().parents[2]
cases = []

def case(n, c, s, o='', p=None):
    cases.append(dict(
        name=n,
        category=c,
        source=s,
        stdout=o,
        phase=p,
        exit=1 if p else 0
    ))

# 1. Pointers
case('pointer_scalar_int', 'pointer',
     '#include <iostream>\nint main(){int x=10;int* p=&x;*p=25;std::cout<<x<<" "<<*p;}',
     '25 25')

case('pointer_scalar_double', 'pointer',
     '#include <iostream>\nint main(){double d=3.5;double* p=&d;*p=7.25;std::cout<<d;}',
     '7.25')

case('pointer_nullptr', 'pointer',
     '#include <iostream>\nint main(){int* p=nullptr;if(p==nullptr)std::cout<<"1";if(p!=nullptr)std::cout<<"2";if(p)std::cout<<"3";else std::cout<<"4";}',
     '14')

case('pointer_arrow_struct', 'pointer',
     '#include <iostream>\nstruct Point{int x;int y;};int main(){Point pt;Point* p=&pt;p->x=12;p->y=34;std::cout<<p->x<<","<<p->y;}',
     '12,34')

case('pointer_to_class', 'pointer',
     '#include <iostream>\nclass Counter{int count;public:Counter(int n):count(n){}void inc(){count++;}int get()const{return count;}};int main(){Counter c(5);Counter* p=&c;p->inc();std::cout<<p->get();} ',
     '6')

case('pointer_param', 'pointer',
     '#include <iostream>\nvoid modify(int* p){*p+=10;}int main(){int a=20;modify(&a);std::cout<<a;}',
     '30')

case('pointer_multi_level', 'pointer',
     '#include <iostream>\nint main(){int x=5;int* p=&x;int** pp=&p;**pp=99;std::cout<<x;}',
     '99')

case('pointer_const_pointee', 'pointer',
     '#include <iostream>\nint main(){int x=42;const int* p=&x;std::cout<<*p;}',
     '42')

case('pointer_const_pointer', 'pointer',
     '#include <iostream>\nint main(){int x=10;int* const p=&x;*p=50;std::cout<<x;}',
     '50')

# 2. Dynamic Storage
case('dynamic_scalar', 'dynamic',
     '#include <iostream>\nint main(){int* p=new int(42);std::cout<<*p;delete p;}',
     '42')

case('dynamic_class', 'dynamic',
     '#include <iostream>\nstruct Resource{int id;Resource(int i):id(i){std::cout<<"C"<<id;}~Resource(){std::cout<<"D"<<id;}};int main(){Resource* r=new Resource(7);std::cout<<"M"<<r->id;delete r;}',
     'C7M7D7')

case('delete_null', 'dynamic',
     '#include <iostream>\nint main(){int* p=nullptr;delete p;std::cout<<"OK";}',
     'OK')

case('dynamic_loop', 'dynamic',
     '#include <iostream>\nint main(){int sum=0;for(int i=1;i<=5;i++){int* p=new int(i*10);sum+=*p;delete p;}std::cout<<sum;}',
     '150')

# 3. RAII & Ownership
case('raii_raw_wrapper', 'raii',
     '#include <iostream>\nclass Box{int* ptr;public:Box(int val):ptr(new int(val)){std::cout<<"C";}~Box(){delete ptr;std::cout<<"D";}int get()const{return *ptr;}};int main(){Box b(100);std::cout<<b.get();}',
     'C100D')

case('raii_linked_list', 'raii',
     '#include <iostream>\nstruct Node{int val;Node* next;Node(int v,Node* n):val(v),next(n){std::cout<<"C"<<val;}~Node(){delete next;std::cout<<"D"<<val;}};int main(){Node* head=new Node(1,new Node(2,nullptr));delete head;}',
     'C2C1D2D1')

case('raii_early_return', 'raii',
     '#include <iostream>\nstruct G{int id;G(int i):id(i){std::cout<<"C"<<id;}~G(){std::cout<<"D"<<id;}};int f(){G g1(1);{G g2(2);return 42;}return 0;}int main(){std::cout<<f();}',
     'C1C2D2D142')

# 4. STL <vector>
case('vector_default', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v;if(v.empty())std::cout<<v.size()<<" empty";else std::cout<<v.size()<<" not empty";}',
     '0 empty')

case('vector_init_list', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={10,20,30};std::cout<<v.size()<<" "<<v[0]<<" "<<v[1]<<" "<<v[2];}',
     '3 10 20 30')

case('vector_push_pop', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v;v.push_back(1);v.push_back(2);v.push_back(3);std::cout<<v.size();v.pop_back();std::cout<<v.size()<<v.back();}',
     '322')

case('vector_front_back', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={100,200,300};std::cout<<v.front()<<" "<<v.back();}',
     '100 300')

case('vector_indexing', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={5,15,25};v[1]=50;std::cout<<v.at(0)<<" "<<v.at(1)<<" "<<v.at(2);}',
     '5 50 25')

case('vector_clear', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={1,2,3};v.clear();if(v.empty())std::cout<<v.size()<<" empty";else std::cout<<v.size()<<" not empty";}',
     '0 empty')

case('vector_copy_ctor', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v1={1,2};std::vector<int> v2=v1;v2.push_back(3);std::cout<<v1.size()<<" "<<v2.size();}',
     '2 3')

case('vector_copy_assign', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v1={10,20};std::vector<int> v2;v2=v1;v2.push_back(30);std::cout<<v1.size()<<" "<<v2.size();}',
     '2 3')

case('vector_iterator_loop', 'stl_vector',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={1,2,3};for(auto it=v.begin();it!=v.end();it++)std::cout<<*it;}',
     '123')

# 5. STL <utility>
case('utility_pair', 'stl_utility',
     '#include <iostream>\n#include <utility>\nint main(){std::pair<int,int> p(10,20);std::cout<<p.first<<" "<<p.second;}',
     '10 20')

case('utility_make_pair', 'stl_utility',
     '#include <iostream>\n#include <utility>\nint main(){auto p=std::make_pair(30,40);std::cout<<p.first<<" "<<p.second;}',
     '30 40')

case('utility_swap', 'stl_utility',
     '#include <iostream>\n#include <utility>\nint main(){int a=1;int b=2;std::swap(a,b);std::cout<<a<<" "<<b;}',
     '2 1')

case('utility_move', 'stl_utility',
     '#include <iostream>\n#include <utility>\nint main(){int a=42;int b=std::move(a);std::cout<<b;}',
     '42')

# 6. STL <algorithm>
case('algo_min_max', 'stl_algorithm',
     '#include <iostream>\n#include <algorithm>\nint main(){std::cout<<std::min(10,20)<<" "<<std::max(10,20);}',
     '10 20')

case('algo_sort', 'stl_algorithm',
     '#include <iostream>\n#include <vector>\n#include <algorithm>\nint main(){std::vector<int> v={30,10,20};std::sort(v.begin(),v.end());for(auto it=v.begin();it!=v.end();it++)std::cout<<*it<<" ";}',
     '10 20 30 ')

case('algo_reverse', 'stl_algorithm',
     '#include <iostream>\n#include <vector>\n#include <algorithm>\nint main(){std::vector<int> v={1,2,3};std::reverse(v.begin(),v.end());for(auto it=v.begin();it!=v.end();it++)std::cout<<*it<<" ";}',
     '3 2 1 ')

case('algo_find', 'stl_algorithm',
     '#include <iostream>\n#include <vector>\n#include <algorithm>\nint main(){std::vector<int> v={10,20,30};auto it=std::find(v.begin(),v.end(),20);if(it!=v.end())std::cout<<*it;}',
     '20')

case('algo_count', 'stl_algorithm',
     '#include <iostream>\n#include <vector>\n#include <algorithm>\nint main(){std::vector<int> v={1,2,2,3,2,4};std::cout<<std::count(v.begin(),v.end(),2);}',
     '3')

# 7. Range-based for
case('range_for_vector_int', 'range_for',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={10,20,30};int sum=0;for(int x:v)sum+=x;std::cout<<sum;}',
     '60')

case('range_for_vector_auto', 'range_for',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={2,3,4};int prod=1;for(auto x:v)prod*=x;std::cout<<prod;}',
     '24')

case('range_for_string', 'range_for',
     '#include <iostream>\n#include <string>\nint main(){std::string s="praxis";int n=0;for(char c:s)n++;std::cout<<n;}',
     '6')

case('accumulator', 'acceptance',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> numbers={10,20,30};int total=0;for(int n:numbers){total+=n;}std::cout<<total;}',
     '60')

# 8. STL <string>
case('string_methods', 'stl_string',
     '#include <iostream>\n#include <string>\nint main(){std::string s="Hello, Praxis!";std::cout<<s.size()<<" "<<s.length()<<" "<<s.substr(7,6)<<" "<<s.at(0);}',
     '14 14 Praxis H')

# 9. Pointer Arithmetic & Comparisons
case('pointer_arithmetic', 'pointer',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={100,200,300};auto p=v.begin();p++;std::cout<<*p<<" ";std::cout<<*(p+1)<<" ";std::cout<<*(p-1);}',
     '200 300 100')

case('pointer_subtraction', 'pointer',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={1,2,3,4};auto p1=v.begin();auto p2=p1+3;std::cout<<(p2-p1);}',
     '3')

case('pointer_relational', 'pointer',
     '#include <iostream>\n#include <vector>\nint main(){std::vector<int> v={1,2,3};auto p1=v.begin();auto p2=p1+1;std::cout<<(p1<p2)<<" "<<(p1>p2)<<" "<<(p1<=p2)<<" "<<(p1>=p2);}',
     '1 0 1 0')

# 10. Adversarial / Rejection cases
case('err_double_free', 'adversarial',
     'int main(){int* p=new int(1);delete p;delete p;}',
     p='runtime')

case('err_delete_stack', 'adversarial',
     'int main(){int x=10;int* p=&x;delete p;}',
     p='runtime')

case('err_use_after_free', 'adversarial',
     'int main(){int* p=new int(10);delete p;return *p;}',
     p='runtime')

case('err_dangling_stack', 'adversarial',
     'int* f(){int x=42;return &x;}int main(){int* p=f();return *p;}',
     p='runtime')

case('err_null_deref', 'adversarial',
     'int main(){int* p=nullptr;return *p;}',
     p='runtime')

case('err_vector_at_bounds', 'adversarial',
     '#include <vector>\nint main(){std::vector<int> v={1,2};return v.at(5);}',
     p='runtime')

case('err_string_at_bounds', 'adversarial',
     '#include <string>\nint main(){std::string s="abc";return s.at(10);}',
     p='runtime')

case('err_deref_non_pointer', 'rejection',
     'int main(){return *5;}',
     p='semantic')

case('err_arrow_non_pointer', 'rejection',
     'struct S{int x;};int main(){S s;return s->x;}',
     p='semantic')

case('err_const_pointee_write', 'rejection',
     'int main(){int x=1;const int* p=&x;*p=2;}',
     p='semantic')

case('err_const_pointer_reassign', 'rejection',
     'int main(){int x=1;int y=2;int* const p=&x;p=&y;}',
     p='semantic')

case('err_delete_non_pointer', 'rejection',
     'int main(){delete 10;}',
     p='semantic')

case('err_address_of_rvalue', 'rejection',
     'int main(){int* p=&(1+2);}',
     p='semantic')

# Write files
fixtures_dir = root / 'tests/m3/fixtures'
fixtures_dir.mkdir(parents=True, exist_ok=True)
for c in cases:
    (fixtures_dir / (c['name'] + '.cpp')).write_text(c['source'], encoding='utf-8')

(root / 'tests/m3/cases.json').write_text(json.dumps(cases, indent=2), encoding='utf-8')

# Also write to examples/m3
examples_dir = root / 'examples/m3'
examples_dir.mkdir(parents=True, exist_ok=True)
for c in cases[:15]:
    (examples_dir / (c['name'] + '.cpp')).write_text(c['source'], encoding='utf-8')

diff_count = sum(c['phase'] is None for c in cases)
print(f"{len(cases)} M3 cases generated ({diff_count} differential candidates)")
