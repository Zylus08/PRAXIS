#include <iostream>
#include <string>
template<typename T>T add(T a,T b){return a+b;}int main(){std::cout<<add<int>(2,3)<<" "<<add<double>(1.5,2.0);}