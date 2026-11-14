#include <iostream>
#include <string>
struct S {int x;S(int n):x(n){std::cout<<"N";}S(const S& s):x(s.x){std::cout<<"C";}~S(){std::cout<<"D";}};S f(){S s(7);return s;}int main(){S a=f();std::cout<<a.x;}