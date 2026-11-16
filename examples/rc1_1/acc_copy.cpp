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
    Accumulator<int> a(10);
    a.add(20);
    Accumulator<int> b = a;
    b.add(30);
    std::cout << a.result() << " " << b.result() << "\n";
    return 0;
}
