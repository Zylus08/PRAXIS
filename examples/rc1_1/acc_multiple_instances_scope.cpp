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
    Accumulator<int> a1(10);
    Accumulator<int> a2(20);
    Accumulator<int> a3(30);

    a1.add(1);
    a2.add(2);
    a3.add(3);

    std::cout << a1.result() << " " << a2.result() << " " << a3.result() << "\n";
    return 0;
}
