#include <iostream>
#include <string>
int f(int x){return 1;}int f(double x){return 2;}int main(){std::cout<<f(1)<<f(1.0)<<f('a');}