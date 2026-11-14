#include <iostream>
#include <string>
template<class T,class U>T choose(T a,U b){return a;}int main(){std::cout<<choose(7,2.5)<<choose<double,int>(1.5,3);}