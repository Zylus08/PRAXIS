#include <iostream>
#include <string>
struct S {int x;S(int n):x(n){}};int get(const S& s){return s.x;}int main(){S a(7);std::cout<<get(a);}