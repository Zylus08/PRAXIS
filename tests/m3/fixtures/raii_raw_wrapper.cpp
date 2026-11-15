#include <iostream>
class Box{int* ptr;public:Box(int val):ptr(new int(val)){std::cout<<"C";}~Box(){delete ptr;std::cout<<"D";}int get()const{return *ptr;}};int main(){Box b(100);std::cout<<b.get();}