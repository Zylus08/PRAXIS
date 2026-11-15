#include <iostream>
#include <vector>
int main(){std::vector<int> v={10,20,30};int sum=0;for(int x:v)sum+=x;std::cout<<sum;}