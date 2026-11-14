#include <iostream>
int f();int a=f();int b[2]={3,4};int f(){return b[1];}int main(){std::cout<<a;}