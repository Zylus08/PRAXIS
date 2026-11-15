#include <iostream>
struct Resource{int id;Resource(int i):id(i){std::cout<<"C"<<id;}~Resource(){std::cout<<"D"<<id;}};int main(){Resource* r=new Resource(7);std::cout<<"M"<<r->id;delete r;}