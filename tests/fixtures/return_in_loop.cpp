#include <iostream>
#include <string>
int f(){for(int i=0;i<4;++i){if(i==2)return i;}return 9;}int main(){std::cout<<f();}