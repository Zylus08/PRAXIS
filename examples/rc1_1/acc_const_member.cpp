#include <iostream>

template<typename T>
class Accumulator {
    T value;
public:
    Accumulator(T initial) : value(initial) {}
    void add(T x) { value += x; }
    T result() const { return value; }
};

int main() {
    const Accumulator<int> a(42);
    std::cout << a.result() << "\n";
    return 0;
}
