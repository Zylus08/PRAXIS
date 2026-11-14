#include <iostream>
#include <string>
template<class T>void set(T& x,T v){x=v;}int main(){int x=1;set(x,9);std::cout<<x;}