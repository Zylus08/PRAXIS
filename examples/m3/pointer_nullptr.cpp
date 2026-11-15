#include <iostream>
int main(){int* p=nullptr;if(p==nullptr)std::cout<<"1";if(p!=nullptr)std::cout<<"2";if(p)std::cout<<"3";else std::cout<<"4";}