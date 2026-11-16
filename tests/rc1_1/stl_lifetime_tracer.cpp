#include <iostream>
#include <vector>

class Tracer {
public:
    int id;
    Tracer() : id(0) {
        std::cout << "C:" << id << " ";
    }
    Tracer(int val) : id(val) {
        std::cout << "C:" << id << " ";
    }
    Tracer(const Tracer& other) : id(other.id) {
        std::cout << "c:" << id << " ";
    }
    ~Tracer() {
        std::cout << "D:" << id << " ";
    }
};

int main() {
    std::cout << "BEGIN\n";
    {
        std::vector<Tracer> v;
        Tracer t1(1);
        Tracer t2(2);
        v.push_back(t1);
        v.push_back(t2);
        std::cout << "POP\n";
        v.pop_back();
        std::cout << "END_INNER\n";
    }
    std::cout << "DONE\n";
    return 0;
}
