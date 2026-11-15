#include "praxis/frontend/parser.hpp"
#include "praxis/frontend/preprocessor.hpp"
#include "praxis/semantics/analyzer.hpp"
#include "praxis/runtime/interpreter.hpp"
#include <iostream>
#include <sstream>

using namespace praxis;

Program parse(const std::string& text, bool script = false) {
    Preprocessor pp;
    auto p = Parser(pp.run({"unit-m3.cpp", text})).parse(script);
    p.iostream = pp.iostream;
    p.strings = pp.string;
    p.vector = pp.vector;
    p.utility = pp.utility;
    p.algorithm = pp.algorithm;
    p.array = pp.array;
    return p;
}

int main() {
    int count = 0;
    auto check = [&](bool value, const char* name) {
        ++count;
        if (!value) {
            std::cerr << "FAIL: " << name << '\n';
            throw std::runtime_error(name);
        }
    };

    try {
        // 1. Pointer type parsing and AST representation
        {
            auto p = parse("int main() { int x = 0; int* a; const int* b; int* const c = &x; int** d; }");
            Analyzer{}.check(p);
            auto fn = p.items[0].function;
            auto s0 = fn->body->children[1];
            check(s0->decl.type.pointer == 1 && s0->decl.type.base == Base::Int, "pointer type parsing");
            auto s1 = fn->body->children[2];
            check(s1->decl.type.pointer == 1 && s1->decl.type.pointeeConstant, "const pointee type parsing");
            auto s2 = fn->body->children[3];
            check(s2->decl.type.pointer == 1 && s2->decl.type.constant, "const pointer type parsing");
            auto s3 = fn->body->children[4];
            check(s3->decl.type.pointer == 2, "multi-level pointer type parsing");
        }

        // 2. Address-of & and dereference *
        {
            auto p = parse("int main() { int a = 10; int* ptr = &a; *ptr = 20; return a; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 20, "address-of and dereference assignment");
        }

        // 3. Arrow -> operator
        {
            auto p = parse("struct Point { int x; int y; }; int main() { Point pt; Point* p = &pt; p->x = 5; p->y = 10; return p->x + p->y; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 15, "arrow member access and assignment");
        }

        // 4. nullptr literal, equality, and condition
        {
            auto p = parse("int main() { int* p = nullptr; int res = 0; if (p == nullptr) res += 1; if (p != nullptr) res += 10; if (p) res += 100; else res += 1000; return res; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 1001, "nullptr comparisons and truthiness");
        }

        // 5. Dynamic allocation & deletion
        {
            auto p = parse("int main() { int* p = new int(42); int val = *p; delete p; return val; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 42, "new scalar and delete");
            check(r.statistics().dynamicAllocations == 1, "dynamic allocation count tracked");
            check(r.statistics().dynamicDeletions == 1, "dynamic deletion count tracked");
        }

        // 6. Safe deletion of nullptr
        {
            auto p = parse("int main() { int* p = nullptr; delete p; return 7; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 7, "null deletion safe");
            check(r.statistics().dynamicDeletions == 0, "null deletion does not increment deallocations");
        }

        // 7. Dynamic class instance construction and destruction
        {
            auto p = parse("class Res { public: int v; Res(int n): v(n) {} ~Res() {} int get() const { return v; } }; int main() { Res* r = new Res(99); int res = r->get(); delete r; return res; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 99, "new class object construction and invocation");
            check(r.statistics().constructions == 1, "dynamic object construction counted");
            check(r.statistics().destructors == 1, "dynamic object destruction counted");
        }

        // 8. Runtime error: Double free
        {
            auto p = parse("int main() { int* p = new int(10); delete p; delete p; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            bool caught = false;
            try {
                r.run(p);
            } catch (const Error& e) {
                caught = (e.phase == "runtime" && std::string(e.what()).find("double free") != std::string::npos);
            }
            check(caught, "double free detected");
        }

        // 9. Runtime error: Deleting non-heap object
        {
            auto p = parse("int main() { int x = 10; int* p = &x; delete p; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            bool caught = false;
            try {
                r.run(p);
            } catch (const Error& e) {
                caught = (e.phase == "runtime" && std::string(e.what()).find("cannot delete non-heap") != std::string::npos);
            }
            check(caught, "deleting automatic stack object rejected");
        }

        // 10. Runtime error: Use after free
        {
            auto p = parse("int main() { int* p = new int(10); delete p; return *p; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            bool caught = false;
            try {
                r.run(p);
            } catch (const Error& e) {
                caught = (e.phase == "runtime" && std::string(e.what()).find("use after free") != std::string::npos);
            }
            check(caught, "use after free detected");
        }

        // 11. Runtime error: Dangling pointer to expired stack frame
        {
            auto p = parse("int* f() { int x = 42; return &x; } int main() { int* p = f(); return *p; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            bool caught = false;
            try {
                r.run(p);
            } catch (const Error& e) {
                caught = (e.phase == "runtime" && std::string(e.what()).find("expired") != std::string::npos);
            }
            check(caught, "dereference of expired stack object detected");
        }

        // 12. Runtime error: Null pointer dereference
        {
            auto p = parse("int main() { int* p = nullptr; return *p; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            bool caught = false;
            try {
                r.run(p);
            } catch (const Error& e) {
                caught = (e.phase == "runtime" && std::string(e.what()).find("null pointer dereference") != std::string::npos);
            }
            check(caught, "null pointer dereference detected");
        }

        // 13. Pointer arithmetic and indexing
        {
            auto p = parse("#include <vector>\nint main() { std::vector<int> v = {10, 20, 30}; int* ptr = v.begin(); ptr++; int v1 = *ptr; ptr--; int diff = (ptr + 2) - ptr; return v1 * 10 + diff; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 202, "pointer arithmetic and subtraction");
        }

        // 14. Vector operations: push_back, pop_back, size, empty, operator[]
        {
            auto p = parse("#include <vector>\nint main() { std::vector<int> v; if (!v.empty()) return 1; v.push_back(10); v.push_back(20); int s1 = v.size(); v.pop_back(); int s2 = v.size(); return s1 * 10 + s2 + v[0]; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 31, "vector push_back pop_back size empty indexing");
        }

        // 15. Vector initializer list constructor
        {
            auto p = parse("#include <vector>\nint main() { std::vector<int> v = {10, 20, 30}; return v.size() + v[0] + v[1] + v[2]; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 63, "vector initializer list constructor");
        }

        // 16. Vector copy construction independence
        {
            auto p = parse("#include <vector>\nint main() { std::vector<int> v1 = {1, 2}; std::vector<int> v2 = v1; v2.push_back(3); return v1.size() * 10 + v2.size(); }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 23, "vector copy construction independence");
        }

        // 17. Vector copy assignment
        {
            auto p = parse("#include <vector>\nint main() { std::vector<int> v1 = {5, 6}; std::vector<int> v2; v2 = v1; return v2.size() + v2[0]; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 7, "vector copy assignment");
        }

        // 18. Pair and make_pair
        {
            auto p = parse("#include <utility>\nint main() { auto p = std::make_pair(10, 20); return p.first + p.second; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 30, "std::pair and make_pair");
        }

        // 19. Swap and min/max
        {
            auto p = parse("#include <utility>\n#include <algorithm>\nint main() { int a = 10; int b = 20; std::swap(a, b); return a * 100 + b + std::min(a, b) + std::max(a, b); }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 2040, "std::swap, min, max");
        }

        // 20. std::sort on vector iterators
        {
            auto p = parse("#include <vector>\n#include <algorithm>\nint main() { std::vector<int> v = {30, 10, 20}; std::sort(v.begin(), v.end()); return v[0] * 100 + v[1] * 10 + v[2]; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 1230, "std::sort on vector iterators");
        }

        // 21. std::reverse on vector iterators
        {
            auto p = parse("#include <vector>\n#include <algorithm>\nint main() { std::vector<int> v = {1, 2, 3}; std::reverse(v.begin(), v.end()); return v[0] * 100 + v[1] * 10 + v[2]; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 321, "std::reverse on vector iterators");
        }

        // 22. std::find and count
        {
            auto p = parse("#include <vector>\n#include <algorithm>\nint main() { std::vector<int> v = {1, 2, 2, 3}; int c = std::count(v.begin(), v.end(), 2); auto it = std::find(v.begin(), v.end(), 3); return c * 10 + *it; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 23, "std::find and std::count");
        }

        // 23. Range-based for over vector
        {
            auto p = parse("#include <vector>\nint main() { std::vector<int> v = {10, 20, 30}; int sum = 0; for (int x : v) sum += x; return sum; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 60, "range-based for loop over vector");
        }

        // 24. Range-based for with auto
        {
            auto p = parse("#include <vector>\nint main() { std::vector<int> v = {1, 2, 3}; int sum = 0; for (auto x : v) sum += x; return sum; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 6, "range-based for loop with auto deduction");
        }

        // 25. Range-based for over string
        {
            auto p = parse("#include <string>\nint main() { std::string s = \"abc\"; int count = 0; for (char c : s) count++; return count; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 3, "range-based for loop over string");
        }

        // 26. String methods: size, length, empty, substr
        {
            auto p = parse("#include <string>\nint main() { std::string s = \"hello\"; int e = 0; if (s.empty()) e = 1; return s.size() + s.length() + e + s.substr(1, 2).size(); }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 12, "string size length empty substr methods");
        }

        // 27. RAII recursive dynamic list
        {
            auto p = parse("struct Node { int val; Node* next; Node(int v, Node* n): val(v), next(n) {} ~Node() { delete next; } }; int main() { Node* head = new Node(1, new Node(2, new Node(3, nullptr))); delete head; return 0; }");
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 0, "recursive RAII list destruction");
            check(r.statistics().dynamicAllocations == 3, "recursive allocations counted");
            check(r.statistics().destructors == 3, "recursive destructors invoked");
            check(r.liveObjectCount() == 0, "all heap objects destroyed");
        }

        // 28. Quota limits on dynamic allocation
        {
            Limits small;
            small.objects = 2;
            auto p = parse("int main() { int* p1 = new int(1); int* p2 = new int(2); int* p3 = new int(3); }");
            Analyzer{}.check(p);
            std::ostringstream sink;
            Interpreter limited(sink, small);
            bool rejected = false;
            try {
                limited.run(p);
            } catch (const Error& e) {
                rejected = (e.phase == "runtime");
            }
            check(rejected, "dynamic allocation quota diagnosed");
        }

        std::cout << count << " M3 unit checks passed\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << diagnostic(e) << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
