#include <iostream>
#include <string>
struct S {int f(){return 1;}int f() const{return 2;}};int main(){S s;const S c;std::cout<<s.f()<<c.f();}