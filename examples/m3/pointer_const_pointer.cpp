#include <iostream>
int main(){int x=10;int* const p=&x;*p=50;std::cout<<x;}