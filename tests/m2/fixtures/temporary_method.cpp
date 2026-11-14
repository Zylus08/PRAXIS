#include <iostream>
#include <string>
struct S {S(){std::cout<<"C";}~S(){std::cout<<"D";}int get() const{return 7;}};int main(){std::cout<<S().get();std::cout<<"X";}