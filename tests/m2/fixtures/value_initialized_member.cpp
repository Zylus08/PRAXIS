#include <iostream>
#include <string>
struct A {int x;};struct B {A a;B():a(){}};int main(){B b;std::cout<<b.a.x;}