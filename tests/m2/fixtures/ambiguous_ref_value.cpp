#include <iostream>
#include <string>
int f(int a){return 1;}int f(int& a){return 2;}int main(){int x=1;f(x);}