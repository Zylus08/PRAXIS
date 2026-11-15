#include <iostream>
#include <vector>
int main(){std::vector<int> v={2,3,4};int prod=1;for(auto x:v)prod*=x;std::cout<<prod;}