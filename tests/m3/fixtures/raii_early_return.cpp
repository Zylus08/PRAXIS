#include <iostream>
struct G{int id;G(int i):id(i){std::cout<<"C"<<id;}~G(){std::cout<<"D"<<id;}};int f(){G g1(1);{G g2(2);return 42;}return 0;}int main(){std::cout<<f();}