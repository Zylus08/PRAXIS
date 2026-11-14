#include <iostream>
#include <string>
struct A {A(){std::cout<<"A";}~A(){std::cout<<"a";}};struct B {A a;B(){int z=0;int x=1/z;}~B(){std::cout<<"b";}};int main(){B b;}