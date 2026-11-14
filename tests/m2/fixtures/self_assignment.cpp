#include <iostream>
#include <string>
struct S {int x;S(int n):x(n){}S(const S& s):x(s.x){std::cout<<"C";}};int main(){S s(4);s=s;std::cout<<s.x;}