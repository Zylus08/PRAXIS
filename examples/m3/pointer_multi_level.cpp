#include <iostream>
int main(){int x=5;int* p=&x;int** pp=&p;**pp=99;std::cout<<x;}