#include <iostream>
#include <string>
struct S {S(int n){std::cout<<n;}S(const S& s){std::cout<<"C";}~S(){std::cout<<"D";}};struct B {S s;B():s(S(3)){}~B(){std::cout<<"B";}};int main(){B b;}