#include <iostream>
#include <string>
template<typename T>T twice(T x){return x+x;}int main(){std::string s="ab";std::cout<<twice(s);}