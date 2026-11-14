#include <iostream>
#include <string>
int f(int x) {
    x = 7;
    return x;
}
int main() {
    int a = 2;
    std::cout << f(a) << a;
}