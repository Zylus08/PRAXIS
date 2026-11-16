#include <iostream>
#include <vector>

template<typename T>
class Accumulator {
    T value;

public:
    Accumulator(T initial) : value(initial) {}

    void add(T x) {
        value += x;
    }

    T result() const {
        return value;
    }
};

int main() {
    std::vector<int> numbers = {10, 20, 30};

    Accumulator<int> sum(0);

    for (int n : numbers) {
        sum.add(n);
    }

    std::cout << sum.result();
}
