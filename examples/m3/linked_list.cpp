#include <iostream>

class Node {
public:
    int val;
    Node* next;
    Node(int v, Node* n) : val(v), next(n) {
        std::cout << 100 + val << "\n";
    }
    ~Node() {
        std::cout << 200 + val << "\n";
        delete next;
    }
};

int main() {
    Node* list = new Node(1, new Node(2, new Node(3, nullptr)));
    Node* curr = list;
    while (curr != nullptr) {
        std::cout << curr->val << " ";
        curr = curr->next;
    }
    std::cout << "\n";
    delete list;
    return 0;
}
