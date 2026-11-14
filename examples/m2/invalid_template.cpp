#include <iostream>
#include <string>
template<class T>T add(T a,T b){return a+b;}struct S {};int main(){S a;add(a,a);}