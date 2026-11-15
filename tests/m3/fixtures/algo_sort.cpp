#include <iostream>
#include <vector>
#include <algorithm>
int main(){std::vector<int> v={30,10,20};std::sort(v.begin(),v.end());for(auto it=v.begin();it!=v.end();it++)std::cout<<*it<<" ";}