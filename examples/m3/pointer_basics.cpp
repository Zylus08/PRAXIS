#include <iostream>

int main() {
    int x = 42;
    int* p = &x;
    std::cout << *p << "\n";
    *p = 100;
    std::cout << x << "\n";
    int* null_p = nullptr;
    if (null_p == nullptr) {
        std::cout << 1 << "\n";
    }
    if (p != nullptr) {
        std::cout << 2 << "\n";
    }
    return 0;
}
