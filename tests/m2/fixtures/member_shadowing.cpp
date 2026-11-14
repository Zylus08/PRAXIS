#include <iostream>
#include <string>
int x=9;struct S {int x;S(int n):x(n){}int f(int x){return x;}int g(){return x;}};int main(){S s(3);std::cout<<s.f(2)<<s.g()<<x;}