#include <iostream>
#include <string>
long long f(int n){if(n<=1)return 1;return n*f(n-1);} int main(){std::cout<<f(10);}