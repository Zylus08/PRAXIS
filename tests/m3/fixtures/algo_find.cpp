#include <iostream>
#include <vector>
#include <algorithm>
int main(){std::vector<int> v={10,20,30};auto it=std::find(v.begin(),v.end(),20);if(it!=v.end())std::cout<<*it;}