#include <iostream>
#include <string>
template<typename T>T twice(T x){return x+x;}int main(){std::cout<<twice(3000000000LL);}