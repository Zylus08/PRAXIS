#include <iostream>
#include <string>
struct G {int n;G(int v):n(v){std::cout<<"C"<<n;}~G(){std::cout<<"D"<<n;}};G a(1);G b(2);int main(){std::cout<<"M";}