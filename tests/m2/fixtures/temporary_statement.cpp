#include <iostream>
#include <string>
struct G {G(int n){std::cout<<n;}~G(){std::cout<<"D";}};int main(){G(3);std::cout<<"X";}