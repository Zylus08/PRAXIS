#include <iostream>
#include <string>
struct L0 {L0(){std::cout<<"0";}~L0(){std::cout<<"0";}};struct L1 {L0 inner;L1(){std::cout<<"1";}~L1(){std::cout<<"1";}};struct L2 {L1 inner;L2(){std::cout<<"2";}~L2(){std::cout<<"2";}};int main(){L2 value;}