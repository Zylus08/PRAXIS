#include <iostream>
#include <string>
struct A {const int x;A():x(1){}};struct B {A a;};int main(){B a;B b;a=b;}