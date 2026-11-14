#include <iostream>
#include <string>
int main(){int a=3;const int& r=a;a=4;std::cout<<r;}
