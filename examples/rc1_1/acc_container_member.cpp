#include <iostream>
#include <vector>

template<typename T>
class HistoryAccumulator {
    std::vector<T> items;
    T total;
public:
    HistoryAccumulator(T init) : total(init) {}
    void add(T x) {
        items.push_back(x);
        total += x;
    }
    T result() const {
        return total;
    }
    int count() const {
        return items.size();
    }
};

int main() {
    HistoryAccumulator<int> acc(10);
    acc.add(20);
    acc.add(30);
    std::cout << acc.result() << " " << acc.count() << "\n";
    return 0;
}
