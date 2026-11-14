#include <iostream>
#include <string>
int main(){int x=3;int& y=x;int& z=y;z++;std::cout<<x;}
