#include <iostream>
#include <utility>
#include <algorithm>

int main() {
    std::pair<int, int> p = std::make_pair(10, 20);
    std::cout << p.first << " " << p.second << "\n";
    int a = 5;
    int b = 15;
    std::swap(a, b);
    std::cout << a << " " << b << "\n";
    std::cout << std::min(a, b) << " " << std::max(a, b) << "\n";
    return 0;
}
