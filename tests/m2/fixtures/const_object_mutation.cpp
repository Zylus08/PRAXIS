#include <iostream>
#include <string>
struct S {int x;S():x(1){}};int main(){const S s;s.x=2;}