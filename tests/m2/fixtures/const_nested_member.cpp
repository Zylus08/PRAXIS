#include <iostream>
#include <string>
struct A {int x;A():x(1){}};struct B {A a;};int main(){const B b;b.a.x=2;}