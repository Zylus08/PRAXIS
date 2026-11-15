#include <iostream>
#include <vector>
#include <algorithm>
int main(){std::vector<int> v={1,2,3};std::reverse(v.begin(),v.end());for(auto it=v.begin();it!=v.end();it++)std::cout<<*it<<" ";}