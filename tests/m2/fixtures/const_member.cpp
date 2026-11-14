#include <iostream>
#include <string>
struct S {int x;S(int n):x(n){}int get() const{return x;}};int main(){const S s(8);std::cout<<s.get();}