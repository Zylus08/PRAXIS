#include <iostream>
#include <string>
struct S {S& operator=(const S& x){return x;}};int main(){}