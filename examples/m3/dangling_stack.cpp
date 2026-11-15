#include <iostream>
int* get_dangling() {
    int x = 123;
    return &x;
}
int main() {
    int* p = get_dangling();
    std::cout << *p << "\n";
    return 0;
}
