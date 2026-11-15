#include <iostream>
int main(){int sum=0;for(int i=1;i<=5;i++){int* p=new int(i*10);sum+=*p;delete p;}std::cout<<sum;}