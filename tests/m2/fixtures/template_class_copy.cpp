#include <iostream>
#include <string>
template<class T>struct Box {T x;Box(T v):x(v){}};int main(){Box<int> a(4);Box<int> b=a;b.x=7;std::cout<<a.x<<b.x;}