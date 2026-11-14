#include <iostream>
#include <string>
template<class T>struct Box {T value;Box(T x):value(x){}};template<class T>T get(const Box<T>& box){return box.value;}int main(){Box<int> b(8);std::cout<<get(b);}