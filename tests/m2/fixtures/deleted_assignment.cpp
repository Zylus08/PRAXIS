#include <iostream>
#include <string>
struct S {const int x;S():x(1){}};int main(){S a;S b;a=b;}