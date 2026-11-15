#include <iostream>
#include <vector>
#include <algorithm>
int main(){std::vector<int> v={1,2,2,3,2,4};std::cout<<std::count(v.begin(),v.end(),2);}