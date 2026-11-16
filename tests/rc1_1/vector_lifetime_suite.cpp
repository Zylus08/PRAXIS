#include <iostream>
#include <vector>

int alive_count = 0;

class Item {
public:
    int id;
    Item() : id(0) {
        alive_count++;
        std::cout << "+I:0 ";
    }
    Item(int v) : id(v) {
        alive_count++;
        std::cout << "+I:" << id << " ";
    }
    Item(const Item& o) : id(o.id) {
        alive_count++;
        std::cout << "+copy:" << id << " ";
    }
    ~Item() {
        alive_count--;
        std::cout << "-I:" << id << " ";
    }
};

void test_resize() {
    std::cout << "\n--- TEST RESIZE ---\n";
    std::vector<Item> v;
    v.resize(3);
    std::cout << "alive=" << alive_count << "\n";
    v.resize(1);
    std::cout << "alive=" << alive_count << "\n";
}

void test_clear() {
    std::cout << "\n--- TEST CLEAR ---\n";
    std::vector<Item> v;
    Item a(10);
    Item b(20);
    v.push_back(a);
    v.push_back(b);
    std::cout << "before_clear alive=" << alive_count << "\n";
    v.clear();
    std::cout << "after_clear alive=" << alive_count << "\n";
}

void test_copy_assign() {
    std::cout << "\n--- TEST COPY ASSIGN ---\n";
    std::vector<Item> v1;
    Item a(1);
    v1.push_back(a);
    
    std::vector<Item> v2;
    Item b(2);
    Item c(3);
    v2.push_back(b);
    v2.push_back(c);

    std::cout << "before_assign alive=" << alive_count << "\n";
    v2 = v1;
    std::cout << "after_assign v2.size=" << v2.size() << " alive=" << alive_count << "\n";
}

int main() {
    test_resize();
    std::cout << "after test_resize alive=" << alive_count << "\n";

    test_clear();
    std::cout << "after test_clear alive=" << alive_count << "\n";

    test_copy_assign();
    std::cout << "after test_copy_assign alive=" << alive_count << "\n";

    std::cout << "\nFINAL alive=" << alive_count << "\n";
    return 0;
}
