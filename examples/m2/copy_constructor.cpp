#include <iostream>
#include <string>
class S {int x;public:S(int n):x(n){}S(const S& s):x(s.x){std::cout<<"C";}int get() const{return x;}};int main(){S a(7);S b=a;std::cout<<b.get();}