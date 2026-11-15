#include <iostream>
void modify(int* p){*p+=10;}int main(){int a=20;modify(&a);std::cout<<a;}