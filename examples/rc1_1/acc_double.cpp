#include <iostream>
#include <vector>

template<typename T>
class Accumulator {
    T value;
public:
    Accumulator(T initial) : value(initial) {}
    void add(T x) { value += x; }
    T result() const { return value; }
};

int main() {
    std::vector<double> numbers = {1.5, 2.5, 3.25};
    Accumulator<double> sum(0.0);
    for (double n : numbers) {
        sum.add(n);
    }
    std::cout << sum.result() << "\n";
    return 0;
}
