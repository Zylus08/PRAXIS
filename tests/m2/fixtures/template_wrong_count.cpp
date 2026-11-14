#include <iostream>
#include <string>
template<class T>T f(T x){return x;}int main(){f<int,double>(1);}