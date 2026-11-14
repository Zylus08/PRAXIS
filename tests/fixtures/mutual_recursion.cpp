#include <iostream>
#include <string>
bool odd(int n);bool even(int n){if(n==0)return true;return odd(n-1);}bool odd(int n){if(n==0)return false;return even(n-1);}int main(){std::cout<<even(8)<<odd(8);}