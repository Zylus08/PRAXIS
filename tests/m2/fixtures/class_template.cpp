#include <iostream>
#include <string>
template<typename T>class Box {T value;public:Box(T x):value(x){}T get() const{return value;}};int main(){Box<int> a(3);Box<double> b(2.5);std::cout<<a.get()<<" "<<b.get();}