#include <iostream>
#include <string>
struct S {int x;S(int n):x(n){}S(const S& s):x(s.x){std::cout<<"C";}};int main(){S a(1);S b=a;b.x=2;a=b;std::cout<<a.x<<b.x;}