#include <iostream>
#include <string>
void f(int &x) {
    x += 7;
}
int main() {
    int a = 2;
    f(a);
    std::cout << a;
}