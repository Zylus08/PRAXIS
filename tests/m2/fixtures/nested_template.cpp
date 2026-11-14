#include <iostream>
#include <string>
template<class T>struct Box {T value;Box(T x):value(x){}T get() const{return value;}};int main(){Box<int> inner(3);Box<Box<int>> outer(inner);std::cout<<outer.value.value;}