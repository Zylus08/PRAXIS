#include <iostream>
#include <string>
struct S {int x;S(int n):x(1){}S(double n):x(2){}};int main(){S a(2);S b(2.0);std::cout<<a.x<<b.x;}