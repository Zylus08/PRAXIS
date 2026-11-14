#include <iostream>
#include <string>
struct S {S(){std::cout<<"C";}~S(){std::cout<<"D";}};int main(){S s((S()));}