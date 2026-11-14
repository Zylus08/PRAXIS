#include <iostream>
#include <string>
struct S {S(int n){std::cout<<n;}S(const S& s){std::cout<<"C";}~S(){std::cout<<"D";}};void f(S s){std::cout<<"F";}int main(){f(S(2));std::cout<<"X";}