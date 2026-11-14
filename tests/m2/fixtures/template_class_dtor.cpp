#include <iostream>
#include <string>
template<class T>struct Box {T x;Box(T v):x(v){}~Box(){std::cout<<x;}};int main(){Box<int> a(4);Box<double> b(2.5);}