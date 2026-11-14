#include <iostream>
#include <string>
struct G {int n;G(int x):n(x){std::cout<<"C"<<n;}~G(){std::cout<<"D"<<n;}};int f(){G a(1);{G b(2);return 7;}}int main(){std::cout<<f();}