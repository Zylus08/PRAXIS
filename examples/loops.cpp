#include <iostream>
int main() {
    for (int i = 0; i < 3; ++i)
        std::cout << i;
    int j = 3;
    while (j < 6)
        std::cout << j++;
    std::cout << std::endl;
}
