#include <iostream>
#include <string>
struct S {int x;S(int n){x=n+1;}};int main(){S s(4);std::cout<<s.x;}