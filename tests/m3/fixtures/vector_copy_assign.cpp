#include <iostream>
#include <vector>
int main(){std::vector<int> v1={10,20};std::vector<int> v2;v2=v1;v2.push_back(30);std::cout<<v1.size()<<" "<<v2.size();}