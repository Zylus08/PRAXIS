#include <iostream>
#include <string>
struct A {A(){std::cout<<"A";}~A(){std::cout<<"a";}};struct B {B(){int z=0;int x=1/z;}~B(){std::cout<<"b";}};struct C {A a;B b;~C(){std::cout<<"c";}};int main(){C c;}