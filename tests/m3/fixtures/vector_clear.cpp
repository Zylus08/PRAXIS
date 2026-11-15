#include <iostream>
#include <vector>
int main(){std::vector<int> v={1,2,3};v.clear();if(v.empty())std::cout<<v.size()<<" empty";else std::cout<<v.size()<<" not empty";}