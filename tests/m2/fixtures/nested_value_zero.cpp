#include <iostream>
#include <string>
struct A {int x;A(){}};struct B {A a;};int main(){B b=B();std::cout<<b.a.x;}