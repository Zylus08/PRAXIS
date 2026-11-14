#include <iostream>
#include <string>
struct A {~A(){std::cout<<"A";}};struct B {~B(){A a;std::cout<<"B";}};int main(){B b;}