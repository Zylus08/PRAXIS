#include <iostream>
#include <vector>

template<typename T>
T sum_container(const std::vector<T>& v) {
    T total = 0;
    for (T x : v) total += x;
    return total;
}

int main() {
    std::vector<int> nums = {1, 2, 3, 4};
    std::cout << sum_container(nums) << "\n";
    return 0;
}
