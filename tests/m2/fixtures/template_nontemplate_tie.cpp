#include <iostream>
#include <string>
template<class T>int f(T x){return 1;}int f(int x){return 2;}int main(){std::cout<<f(2)<<f(2.0)<<f<int>(2);}