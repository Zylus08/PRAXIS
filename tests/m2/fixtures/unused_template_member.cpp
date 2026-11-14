#include <iostream>
#include <string>
template<class T>struct S {T x;int f(){return x.bad;}};int main(){S<int> s;s.x=3;std::cout<<s.x;}