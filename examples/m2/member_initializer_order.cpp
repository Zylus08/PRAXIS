#include <iostream>
#include <string>
struct G {int n;G(int x):n(x){std::cout<<n;}~G(){std::cout<<n;}};struct S {G a;G b;S():b(2),a(1){std::cout<<"P";}~S(){std::cout<<"Q";}};int main(){S s;}