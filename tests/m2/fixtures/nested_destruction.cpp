#include <iostream>
#include <string>
struct A {A(){std::cout<<"a";}~A(){std::cout<<"A";}};struct B {A a;B(){std::cout<<"b";}~B(){std::cout<<"B";}};int main(){B b;}