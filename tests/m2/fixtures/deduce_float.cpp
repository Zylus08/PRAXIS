#include <iostream>
#include <string>
template<typename T>T twice(T x){return x+x;}int main(){std::cout<<twice(1.25f);}