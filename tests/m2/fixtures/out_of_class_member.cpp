#include <iostream>
#include <string>
class S {int x;public:S(int n):x(n){}int get() const;};int S::get() const{return x;}int main(){S s(6);std::cout<<s.get();}