#define A 1
#undef A
#define A 2
#include <iostream>
int main(){std::cout<<A;}