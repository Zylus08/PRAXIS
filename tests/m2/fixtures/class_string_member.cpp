#include <iostream>
#include <string>
class S {std::string s;public:S(std::string v):s(v){}std::string get() const{return s;}};int main(){S s("hi");std::cout<<s.get();}