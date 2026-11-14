#include <iostream>
#include <string>
struct S {int x;S(){std::cout<<x;}};S s;int main(){std::cout<<s.x;}