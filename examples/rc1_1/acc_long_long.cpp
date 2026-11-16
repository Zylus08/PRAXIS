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
    std::vector<long long> numbers = {10000000000LL, 20000000000LL, 30000000000LL};
    Accumulator<long long> sum(0LL);
    for (long long n : numbers) {
        sum.add(n);
    }
    std::cout << sum.result() << "\n";
    return 0;
}
