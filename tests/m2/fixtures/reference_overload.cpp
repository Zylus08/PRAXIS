#include <iostream>
#include <string>
int f(int& x){return 1;}int f(const int& x){return 2;}int main(){int x=1;const int y=1;std::cout<<f(x)<<f(y);}