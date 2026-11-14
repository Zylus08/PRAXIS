#include <iostream>
const int c=3;int f();int a=f();int b=c*2;int f(){return b;}int main(){std::cout<<a<<b;}