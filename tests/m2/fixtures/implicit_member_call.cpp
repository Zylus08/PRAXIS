#include <iostream>
#include <string>
struct S {int x=3;int a() const{return x;}int b() const{return a()+1;}};int main(){S s;std::cout<<s.b();}