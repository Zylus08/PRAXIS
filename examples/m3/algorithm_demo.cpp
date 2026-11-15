#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> v = {30, 10, 50, 20, 40};
    std::sort(v.begin(), v.end());
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << "\n";
    std::reverse(v.begin(), v.end());
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << "\n";
    int c = std::count(v.begin(), v.end(), 30);
    std::cout << "count:" << c << "\n";
    return 0;
}
