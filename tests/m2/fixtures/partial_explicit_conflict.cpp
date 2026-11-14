#include <iostream>
#include <string>
template<class T,class U>U f(T a,U b,U c){return b;}int main(){f<int>(1,2,3.0);}