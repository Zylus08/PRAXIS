#include <iostream>
#include <vector>
int main(){std::vector<int> v={1,2,3,4};auto p1=v.begin();auto p2=p1+3;std::cout<<(p2-p1);}