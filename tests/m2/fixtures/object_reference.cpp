#include <iostream>
#include <string>
struct S {int x;S(int n):x(n){}};void set(S& s){s.x=8;}int main(){S a(1);S& b=a;set(b);std::cout<<a.x;}