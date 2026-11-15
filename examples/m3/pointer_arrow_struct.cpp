#include <iostream>
struct Point{int x;int y;};int main(){Point pt;Point* p=&pt;p->x=12;p->y=34;std::cout<<p->x<<","<<p->y;}