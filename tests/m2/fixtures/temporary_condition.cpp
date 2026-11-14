#include <iostream>
#include <string>
struct S {S(){std::cout<<"C";}~S(){std::cout<<"D";}bool test() const{return true;}};int main(){if(S().test())std::cout<<"B";}