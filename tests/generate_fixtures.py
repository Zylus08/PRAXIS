from pathlib import Path
import json
cases=[]
def add(name,category,body,expected='',exit=0,phase=None,raw=False):
 source=body if raw else '#include <iostream>\n#include <string>\nint main(){'+body+'}\n'
 cases.append(dict(name=name,category=category,source=source,stdout=expected,exit=exit,phase=phase))
def expr(name,expression,expected,category='arithmetic'):
 add(name,category,'std::cout << ('+expression+');',str(expected))
for name,e,v in [
 ('add','17+25',42),('subtract','17-25',-8),('multiply','7*6',42),('divide','43/6',7),('remainder','43%6',1),('negative_divide','-43/6',-7),('negative_remainder','-43%6',-1),('precedence','2+3*4',14),('parentheses','(2+3)*4',20),('left_subtract','20-6-3',11),('left_divide','48/4/3',4),('unary_minus','-(-12)',12),('unary_plus','+12',12),('long_literal','3000000000+2',3000000002),('long_suffix','42LL*1000000000LL',42000000000),('mixed_rank','3+4LL',7),('bool_promotion','true+true',2),('char_promotion',"'A'+1",66),('float_add','1.5f+2.25f','3.75'),('double_divide','5.0/2','2.5'),('exponent','1e2+2.5e-1','100.25'),('leading_dot','.5+1.','1.5'),('float_double','1.25f+2.5','3.75'),('less','3<4',1),('greater','3>4',0),('less_equal','3<=3',1),('greater_equal','3>=3',1),('equal','3==3',1),('not_equal','3!=3',0),('logical_and','true&&false',0),('logical_or','false||true',1),('logical_not','!42',0),('not_zero','!0',1),('compare_precedence','1+2==3&&4>2',1),('logical_precedence','true||false&&false',1),('long_compare','9007199254740993LL>9007199254740992LL',1),('float_round','16777217.0f==16777216.0f',1),('negative_zero','-0.0==0.0',1)]:expr(name,e,v)
valid=[
 ('assignment','assignment','int a=0; int b=0; a=b=7; std::cout<<a<<b;','77'),
 ('add_assign','assignment','int a=4;a+=3;std::cout<<a;','7'),('sub_assign','assignment','int a=4;a-=7;std::cout<<a;','-3'),('mul_assign','assignment','int a=4;a*=3;std::cout<<a;','12'),('div_assign','assignment','int a=10;a/=3;std::cout<<a;','3'),('mod_assign','assignment','int a=10;a%=3;std::cout<<a;','1'),
 ('prefix_increment','assignment','int a=2;std::cout<<++a<<a;','33'),('postfix_increment','assignment','int a=2;std::cout<<a++<<a;','23'),('prefix_decrement','assignment','int a=2;std::cout<<--a<<a;','11'),('postfix_decrement','assignment','int a=2;std::cout<<a--<<a;','21'),('assignment_lvalue','assignment','int a=0;(a=2)=3;std::cout<<a;','3'),
 ('int_from_double','conversion','int a=3.9;std::cout<<a;','3'),('negative_truncate','conversion','int a=-3.9;std::cout<<a;','-3'),('bool_from_int','conversion','bool a=9;bool b=0;std::cout<<a<<b;','10'),('bool_from_double','conversion','bool a=.5;std::cout<<a;','1'),('char_output','conversion',"char c=65;std::cout<<c;",'A'),('float_from_int','conversion','float f=5;std::cout<<f/2;','2.5'),
 ('if_true','control','if(2)std::cout<<1;else std::cout<<0;','1'),('if_false','control','if(0)std::cout<<1;else std::cout<<0;','0'),('dangling_else','control','if(true)if(false)std::cout<<1;else std::cout<<2;','2'),
 ('while','loops','int i=0;while(i<4){std::cout<<i;i++;}','0123'),('do_once','loops','int i=4;do{std::cout<<i;i++;}while(i<4);','4'),('do_loop','loops','int i=0;do{std::cout<<i;i++;}while(i<3);','012'),('for','loops','for(int i=0;i<4;++i)std::cout<<i;','0123'),('for_empty','loops','int i=0;for(;;){if(i==3)break;std::cout<<i++;}','012'),('while_break','loops','int i=0;while(true){if(i==2)break;std::cout<<i++;}','01'),('for_continue','loops','for(int i=0;i<5;++i){if(i==2)continue;std::cout<<i;}','0134'),('while_continue','loops','int i=0;while(i<4){i++;if(i==2)continue;std::cout<<i;}','134'),('do_continue','loops','int i=0;do{i++;if(i==2)continue;std::cout<<i;}while(i<3);','13'),('nested_break','loops','for(int i=0;i<2;++i){for(int j=0;j<3;++j){if(j==1)break;std::cout<<i<<j;}}','0010'),
 ('shadow','scope','int x=1;{int x=2;std::cout<<x;}std::cout<<x;','21'),('block_mutate','scope','int x=1;{x=3;}std::cout<<x;','3'),('for_scope','scope','int i=9;for(int i=0;i<2;++i)std::cout<<i;std::cout<<i;','019'),('branch_scope','scope','int x=1;if(true){int x=2;std::cout<<x;}std::cout<<x;','21'),
 ('array_list','array','int a[3]={2,4,6};std::cout<<a[0]<<a[2];','26'),('array_zero_fill','array','int a[3]={2};std::cout<<a[1]<<a[2];','00'),('array_empty_list','array','int a[2]={};std::cout<<a[0]<<a[1];','00'),('array_write','array','int a[2];a[1]=7;std::cout<<a[1];','7'),('array_loop','array','int a[4]={1,2,3,4};int s=0;for(int i=0;i<4;++i)s+=a[i];std::cout<<s;','10'),('array_char','array',"char a[2]={'a','b'};std::cout<<a[0]<<a[1];",'ab'),
 ('reference','reference','int x=3;int& y=x;y=8;std::cout<<x;','8'),('reference_chain','reference','int x=3;int& y=x;int& z=y;z++;std::cout<<x;','4'),('reference_array','reference','int a[2]={1,2};int& r=a[1];r=9;std::cout<<a[1];','9'),('const_reference','reference','int a=3;const int& r=a;a=4;std::cout<<r;','4'),('copy_value','reference','int a=3;int b=a;b=4;std::cout<<a<<b;','34'),
 ('string_default','string','std::string s;std::cout<<s.empty()<<s.size();','10'),('string_literal','string','std::string s="hello";std::cout<<s<<s.size();','hello5'),('string_plus','string','std::string s="hi";std::cout<<(s+"!");','hi!'),('string_prepend','string','std::string s="hi";std::cout<<("!"+s);','!hi'),('string_append','string','std::string s="hi";s+="!";std::cout<<s;','hi!'),('string_char','string',"std::string s=\"hi\";s+='!';std::cout<<s;",'hi!'),('string_copy','string','std::string s="a";std::string t=s;t+="b";std::cout<<s<<t;','aab'),('string_compare','string','std::string s="abc";std::cout<<(s=="abc")<<(s<"abd");','11'),('string_length','string','std::string s="abc";std::cout<<s.length();','3'),('short_and','control','int x=0;false&&++x;std::cout<<x;','0'),('short_or','control','int x=0;true||++x;std::cout<<x;','0'),('endl','native','std::cout<<"hello"<<std::endl;','hello\n'),('escape','lexer','std::cout<<"a\\tb\\n";','a\tb\n'),('comments','lexer','/* a\n b */ int x=2; // c\nstd::cout<<x;','2')]
for n,c,b,o in valid:add(n,c,b,o)
funcs=[
 ('factorial','long long f(int n){if(n<=1)return 1;return n*f(n-1);} int main(){std::cout<<f(10);}','3628800'),
 ('fibonacci','int f(int n){if(n<2)return n;return f(n-1)+f(n-2);} int main(){std::cout<<f(10);}','55'),
 ('pass_value','int f(int x){x=7;return x;}int main(){int a=2;std::cout<<f(a)<<a;}','72'),
 ('pass_reference','void f(int& x){x+=7;}int main(){int a=2;f(a);std::cout<<a;}','9'),
 ('const_parameter','int f(const int& x){return x;}int main(){int a=2;std::cout<<f(a);}','2'),
 ('parameter_convert','int f(int x){return x;}int main(){std::cout<<f(2.9);}','2'),
 ('return_convert','int f(){return 2.9;}int main(){std::cout<<f();}','2'),
 ('void_return','void f(){std::cout<<1;return;}int main(){f();}','1'),
 ('forward_declaration','int f(int n);int main(){std::cout<<f(3);}int f(int n){return n*2;}','6'),
 ('mutual_recursion','bool odd(int n);bool even(int n){if(n==0)return true;return odd(n-1);}bool odd(int n){if(n==0)return false;return even(n-1);}int main(){std::cout<<even(8)<<odd(8);}','10'),
 ('global_scope','int x=3;int f(){return x;}int main(){int x=9;std::cout<<f()<<x;}','39'),
 ('global_zero','int x;int a[2];int main(){std::cout<<x<<a[1];}','00'),
 ('return_in_loop','int f(){for(int i=0;i<4;++i){if(i==2)return i;}return 9;}int main(){std::cout<<f();}','2'),
 ('string_function','std::string f(std::string s){return s+"!";}int main(){std::cout<<f("hi");}','hi!'),
 ('parameter_shadow','int f(int x){{int x=7;std::cout<<x;}return x;}int main(){std::cout<<f(2);}','72')]
for n,b,o in funcs:add(n,'function','#include <iostream>\n#include <string>\n'+b,o,raw=True)
add('return_status','cli','return 7;','',7)
add('macro','preprocess','#define N 6\n#include <iostream>\nint main(){std::cout<<N*7;}','42',raw=True)
add('macro_nested','preprocess','#define A B\n#define B 42\n#include <iostream>\nint main(){std::cout<<A;}','42',raw=True)
add('macro_undef','preprocess','#define A 1\n#undef A\n#define A 2\n#include <iostream>\nint main(){std::cout<<A;}','2',raw=True)
add('macro_literal','preprocess','#define A 7\n#include <iostream>\nint main(){std::cout<<"A";}','A',raw=True)
invalid=[('unknown_name','x=1;'),('duplicate_local','int x;int x;'),('const_assignment','const int x=1;x=2;'),('rvalue_assignment','1=2;'),('array_assignment','int a[2];int b[2];a=b;'),('string_to_int','int x="bad";'),('int_to_string','std::string s=42;'),('reference_temporary','int& r=3;'),('reference_conversion','double d=3;int& r=d;'),('reference_const','const int x=1;int& r=x;'),('missing_reference_init','int& x;'),('missing_const_init','const int x;'),('void_object','void x;'),('float_index','int a[2];a[1.5]=2;'),('scalar_index','int a;a[0]=2;'),('bool_increment','bool x=true;++x;'),('float_remainder','double x=2.0;std::cout<<(x%2);'),('break_outside','break;'),('continue_outside','continue;'),('scope_escape','{int x=1;}std::cout<<x;'),('for_escape','for(int i=0;i<1;i++){}std::cout<<i;'),('string_condition','std::string s="x";if(s){}'),('const_ref_write','int x=1;const int& r=x;r=3;'),('array_too_many','int a[2]={1,2,3};'),('array_narrow','int a[1]={1.5};'),('array_nonbrace','int a[2]=1;'),('bad_member','std::string s;s.clear();'),('member_on_int','int x=1;x.size();'),('integer_shift','int x=1;std::cout<<(x<<2);'),('text_plus_text','std::cout<<("a"+"b");'),('return_missing','return;'),('array_of_refs','int& a[2];'),('call_main','main();')]
for n,b in invalid:add(n,'semantic_rejection',b,exit=1,phase='semantic')
for n,b in [('bad_arity','int f(int x){return x;}int main(){f();}'),('undefined_function','int f(int x);int main(){f(1);}'),('missing_return','int f(int x){if(x)return 1;}int main(){}'),('duplicate_function','int f(){return 1;}int f(){return 2;}int main(){}'),('overload','int f(int x){return x;}int f(double x){return 1;}int main(){}'),('parameter_redeclare','int f(int x){int x=1;return x;}int main(){}'),('before_declaration','int main(){f();}void f(){}'),('missing_main','int x=1;'),('invalid_main','void main(){}'),('return_void_value','void f(){return 1;}int main(){}')]:add(n,'semantic_rejection',b,exit=1,phase='semantic',raw=True)
for n,b in [('missing_semicolon','int x=1'),('missing_brace','if(true){'),('bad_expression','int x=;'),('array_bound_zero','int a[0];'),('array_bound_name','int n=2;int a[n];'),('pointer','int* p;'),('scalar_braces','int x{1};'),('missing_paren','if(true std::cout<<1;')]:add(n,'syntax_rejection',b,exit=1,phase='parse')
for n,b in [('unterminated_string','"bad'),('unterminated_comment','/*bad'),('bad_escape','"\\q"'),('multi_character',"'ab'"),('octal','int x=012;'),('hex','int x=0xff;'),('bad_exponent','double x=1e;'),('unsupported_character','@'),('trailing_escape','"x\\')]:add(n,'lex_rejection',b,exit=1,phase='lex',raw=True)
for n,b in [('unsupported_directive','#if 1\nint main(){}\n#endif'),('function_macro','#define F(x) x\nint main(){}'),('unknown_header','#include <vector>\nint main(){}'),('redefine','#define X 1\n#define X 2\nint main(){}')]:add(n,'preprocess_rejection',b,exit=1,phase='preprocess',raw=True)
for n,b in [('uninitialized','int x;std::cout<<x;'),('array_uninitialized','int a[2];std::cout<<a[0];'),('bounds_high','int a[2]={};std::cout<<a[2];'),('bounds_negative','int a[2]={};a[-1]=0;'),('divide_zero','int x=0;std::cout<<(3/x);'),('mod_zero','int x=0;std::cout<<(3%x);'),('float_divide_zero','double x=0;std::cout<<(3.0/x);'),('int_add_overflow','int x=2147483647;x+=1;'),('int_sub_overflow','int x=-2147483647;x-=2;'),('int_mul_overflow','int x=1000000;x*=1000000;'),('long_add_overflow','long long x=9223372036854775807LL;x+=1;'),('long_mul_overflow','long long x=9223372036854775807LL;x*=2;'),('long_div_overflow','long long x=-9223372036854775807LL-1;std::cout<<(x/-1);'),('long_neg_overflow','long long x=-9223372036854775807LL-1;std::cout<<-x;'),('conversion_range','double d=1e30;int x=d;'),('char_range','int x=200;char c=x;'),('self_init','int x=x;'),('floating_overflow','double x=1e308;std::cout<<x*x;')]:add(n,'runtime_rejection',b,exit=1,phase='runtime')

for n,text in [('literal_bang','!'),('literal_plus','+'),('literal_parenthesis','('),('literal_brace','}'),('literal_semicolon',';'),('literal_keyword','int'),('literal_hash','#')]:add(n,'lexer','std::cout<<"'+text+'";',text)
add('zero_before_dynamic_init','scope','#include <iostream>\nint f();int a=f();int b=7;int f(){return b;}int main(){std::cout<<a<<b;}','77',raw=True)
add('nul_literal_output','native','std::cout<<"a\\0b";','a')
add('nul_string_conversion','string','std::string s="a\\0b";std::cout<<s.size();','1')
add('nul_string_concat','string','std::string s="x";std::cout<<(s+"a\\0b");','xa')
add('endl_not_string','semantic_rejection','std::string s=std::endl;',exit=1,phase='semantic')
add('long_expression','syntax_rejection','std::cout<<('+ '+'.join(['1']*140)+');',exit=1,phase='parse')
add('call_depth','runtime_rejection','int f(){return f();}int main(){return f();}',exit=1,phase='runtime',raw=True)

add('include_order','semantic_rejection','int main(){std::cout<<1;}\n#include <iostream>\n',exit=1,phase='semantic',raw=True)
add('for_redeclare','semantic_rejection','for(int i=0;i<1;++i){int i=2;}',exit=1,phase='semantic')
add('keyword_identifier','syntax_rejection','int class=1;',exit=1,phase='parse')
add('std_identifier','semantic_rejection','int std=1;',exit=1,phase='semantic')

add('constant_global_chain','scope','#include <iostream>\nconst int c=3;int f();int a=f();int b=c*2;int f(){return b;}int main(){std::cout<<a<<b;}','66',raw=True)
add('constant_global_array','scope','#include <iostream>\nint f();int a=f();int b[2]={3,4};int f(){return b[1];}int main(){std::cout<<a;}','4',raw=True)
add('static_reference_binding','reference','#include <iostream>\nint f();int a=f();int b=7;int& r=b;int f(){return r;}int main(){std::cout<<a;}','7',raw=True)
add('for_nested_shadow','scope','for(int i=0;i<1;++i){{int i=7;std::cout<<i;}}','7')

add('undef_requires_name','preprocess_rejection','#undef 3\nint main(){}',exit=1,phase='preprocess',raw=True)
add('directive_requires_name','preprocess_rejection','# \"define\" X\nint main(){}',exit=1,phase='preprocess',raw=True)
add('negative_zero_output','conversion','std::cout<<-0.0;', '-0')
add('long_to_float_rounding','conversion','long long x=4611686293305294849LL;float f=x;std::cout<<(f>4611686018427387904.0f);','1')
root=Path('tests/fixtures');root.mkdir(parents=True,exist_ok=True)
for c in cases:(root/(c['name']+'.cpp')).write_text(c['source'])
Path('tests/fixtures/cases.json').write_text(json.dumps(cases,indent=2))
print(len(cases),'fixtures;',sum(c['phase'] is None for c in cases),'differential candidates')
