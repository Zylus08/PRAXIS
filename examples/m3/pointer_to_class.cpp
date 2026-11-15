#include <iostream>
class Counter{int count;public:Counter(int n):count(n){}void inc(){count++;}int get()const{return count;}};int main(){Counter c(5);Counter* p=&c;p->inc();std::cout<<p->get();} 