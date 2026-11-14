#include <iostream>
#include <string>
class C {int n;public:C(int x):n(x){}void add(int x){n+=x;}int get() const{return n;}};int main(){C c(2);c.add(3);std::cout<<c.get();}