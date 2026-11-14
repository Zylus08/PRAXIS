#include <iostream>
#include <string>
struct S {int x;S():x(){}};int main(){S a;std::cout<<a.x;}