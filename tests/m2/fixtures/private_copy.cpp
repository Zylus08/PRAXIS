#include <iostream>
#include <string>
class S {S(const S& s){}public:S(){}};int main(){S a;S b=a;}