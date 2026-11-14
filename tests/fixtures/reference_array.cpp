#include <iostream>
#include <string>
int main(){int a[2]={1,2};int& r=a[1];r=9;std::cout<<a[1];}
