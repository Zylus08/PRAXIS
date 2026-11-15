#include <iostream>
#include <string>

int main() {
    std::string s = "PRAXIS";
    for (char c : s) {
        std::cout << c << " ";
    }
    std::cout << "\n";
    return 0;
}
