#include <iostream>
#include <vector>

int main() {
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = v1;
    v2.push_back(4);
    std::cout << v1.size() << " " << v2.size() << "\n";
    std::vector<int> v3;
    v3 = v2;
    std::cout << v3.size() << " " << v3[3] << "\n";
    return 0;
}
