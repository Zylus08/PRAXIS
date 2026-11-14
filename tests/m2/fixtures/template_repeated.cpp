#include <iostream>
#include <string>
template<class T>T twice(T a){return a+a;}int main(){std::cout<<twice(2)<<twice(3)<<twice(1.5);}