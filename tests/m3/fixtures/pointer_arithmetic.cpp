#include <iostream>
#include <vector>
int main(){std::vector<int> v={100,200,300};auto p=v.begin();p++;std::cout<<*p<<" ";std::cout<<*(p+1)<<" ";std::cout<<*(p-1);}