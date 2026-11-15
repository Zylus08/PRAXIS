#include <iostream>
int main(){int x=10;int* p=&x;*p=25;std::cout<<x<<" "<<*p;}