#include <iostream>
#include <string>
struct S {void f(){}};int main(){const S s;s.f();}