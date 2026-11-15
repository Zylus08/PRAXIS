#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;
    std::cout << v.empty() << " " << v.size() << "\n";
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    std::cout << v.size() << " " << v.front() << " " << v.back() << "\n";
    std::cout << v[1] << " " << v.at(2) << "\n";
    v.pop_back();
    std::cout << v.size() << " " << v.back() << "\n";
    v.clear();
    std::cout << v.empty() << " " << v.size() << "\n";
    return 0;
}
