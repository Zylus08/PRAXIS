#include <iostream>
#include <string>
struct S {int x;S(int n):x(n){}S(const S& s):x(s.x){std::cout<<"C";}~S(){std::cout<<"D"<<x;}};void f(S s){s.x=8;}int main(){S s(1);f(s);std::cout<<s.x;}