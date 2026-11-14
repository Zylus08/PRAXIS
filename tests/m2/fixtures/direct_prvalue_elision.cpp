#include <iostream>
#include <string>
struct S {S(int n){std::cout<<n;}S(const S& s){std::cout<<"C";}~S(){std::cout<<"D";}};int main(){S s(S(3));}