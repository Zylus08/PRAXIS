#include <iostream>
int f();int a=f();int b=7;int f(){return b;}int main(){std::cout<<a<<b;}