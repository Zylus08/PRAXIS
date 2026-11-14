#include <iostream>
#include <string>
struct L0 {L0(){std::cout<<"0";}~L0(){std::cout<<"0";}};struct L1 {L0 inner;L1(){std::cout<<"1";}~L1(){std::cout<<"1";}};int main(){L1 value;}