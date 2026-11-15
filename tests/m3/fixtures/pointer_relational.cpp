#include <iostream>
#include <vector>
int main(){std::vector<int> v={1,2,3};auto p1=v.begin();auto p2=p1+1;std::cout<<(p1<p2)<<" "<<(p1>p2)<<" "<<(p1<=p2)<<" "<<(p1>=p2);}