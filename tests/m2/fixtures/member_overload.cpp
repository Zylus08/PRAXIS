#include <iostream>
#include <string>
struct S {int f(int x){return 1;}int f(double x){return 2;}};int main(){S s;std::cout<<s.f(2)<<s.f(2.0);}