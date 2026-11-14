#include <iostream>
#include <string>
struct A {~A(){std::cout<<"A";}};struct B {A a;~B(){std::cout<<"B";int z=0;int x=1/z;}};int main(){A first;B second;}