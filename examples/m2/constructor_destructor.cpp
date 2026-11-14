#include <iostream>
#include <string>
struct G {int n;G(int x):n(x){std::cout<<"C"<<n;}~G(){std::cout<<"D"<<n;}};int main(){G a(1);{G b(2);}std::cout<<"X";}