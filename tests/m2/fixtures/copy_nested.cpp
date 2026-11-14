#include <iostream>
#include <string>
struct A {int x;A(int n):x(n){}A(const A& a):x(a.x){std::cout<<"C";}};struct B {A a;B(int n):a(n){}};int main(){B a(3);B b=a;b.a.x=9;std::cout<<a.a.x<<b.a.x;}