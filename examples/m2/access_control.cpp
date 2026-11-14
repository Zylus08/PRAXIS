#include <iostream>
#include <string>
class S {int x;int hidden() const{return x;}public:S(int n):x(n){}int get() const{return hidden();}};int main(){S s(9);std::cout<<s.get();}