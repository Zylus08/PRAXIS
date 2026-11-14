#include <iostream>
#include <string>
template<class T>int f(T x){return x.missing;}int f(int x){return 7;}int main(){std::cout<<f(1);}