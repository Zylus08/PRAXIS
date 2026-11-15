#include <iostream>

class UniqueInt {
    int* ptr;
public:
    UniqueInt(int val) : ptr(new int(val)) {
        std::cout << "created:" << *ptr << "\n";
    }
    ~UniqueInt() {
        if (ptr != nullptr) {
            std::cout << "destroyed:" << *ptr << "\n";
            delete ptr;
            ptr = nullptr;
        }
    }
    int get() const {
        return *ptr;
    }
    void set(int val) {
        *ptr = val;
    }
};

int main() {
    {
        UniqueInt u(42);
        std::cout << "value:" << u.get() << "\n";
        u.set(100);
        std::cout << "updated:" << u.get() << "\n";
    }
    std::cout << "done\n";
    return 0;
}
