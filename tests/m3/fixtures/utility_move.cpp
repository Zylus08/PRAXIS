#include <iostream>
#include <utility>
int main(){int a=42;int b=std::move(a);std::cout<<b;}