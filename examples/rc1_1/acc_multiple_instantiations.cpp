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
    Accumulator<int> a(10);
    a.add(20);
    Accumulator<double> b(1.5);
    b.add(2.5);
    Accumulator<long long> c(100LL);
    c.add(200LL);

    std::cout << a.result() << " " << b.result() << " " << c.result() << "\n";
    return 0;
}
