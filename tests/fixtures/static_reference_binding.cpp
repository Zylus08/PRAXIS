#include <iostream>
int f();int a=f();int b=7;int& r=b;int f(){return r;}int main(){std::cout<<a;}