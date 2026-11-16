#include "praxis/frontend/parser.hpp"
#include "praxis/frontend/preprocessor.hpp"
#include "praxis/semantics/analyzer.hpp"
#include "praxis/runtime/interpreter.hpp"
#include <iostream>
#include <sstream>

using namespace praxis;

static Program parseCode(const std::string& text) {
    Preprocessor pp;
    auto p = Parser(pp.run({"unit-rc1_1.cpp", text})).parse(false);
    p.iostream = pp.iostream;
    p.strings = pp.string;
    p.vector = pp.vector;
    p.utility = pp.utility;
    p.algorithm = pp.algorithm;
    p.array = pp.array;
    return p;
}

int main() {
    int total = 0;
    auto check = [&](bool condition, const char* name) {
        ++total;
        if (!condition) {
            std::cerr << "FAIL: " << name << "\n";
            throw std::runtime_error(name);
        }
    };

    try {
        // 1. Mandatory Original Acceptance Program
        std::cout << "Test 1\n";
        {
            auto p = parseCode(
                "#include <vector>\n"
                "template<typename T>\n"
                "class Accumulator {\n"
                "    T value;\n"
                "public:\n"
                "    Accumulator(T initial) : value(initial) {}\n"
                "    void add(T x) { value += x; }\n"
                "    T result() const { return value; }\n"
                "};\n"
                "int main() {\n"
                "    std::vector<int> numbers = {10, 20, 30};\n"
                "    Accumulator<int> sum(0);\n"
                "    for (int n : numbers) {\n"
                "        sum.add(n);\n"
                "    }\n"
                "    return sum.result();\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 60, "original accumulator template with vector range-for");
            check(r.liveObjectCount() == 0, "accumulator zero live objects at exit");
        }

        // 2. Class template with container member
        std::cout << "Test 2\n";
        {
            auto p = parseCode(
                "#include <vector>\n"
                "template<typename T>\n"
                "class HistoryAccumulator {\n"
                "    std::vector<T> items;\n"
                "    T total;\n"
                "public:\n"
                "    HistoryAccumulator(T init) : total(init) {}\n"
                "    void add(T x) { items.push_back(x); total += x; }\n"
                "    T result() const { return total; }\n"
                "    int count() const { return items.size(); }\n"
                "};\n"
                "int main() {\n"
                "    HistoryAccumulator<int> acc(10);\n"
                "    acc.add(20);\n"
                "    acc.add(30);\n"
                "    return acc.result() * 10 + acc.count();\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 602, "template class with std::vector member");
            check(r.liveObjectCount() == 0, "history accumulator zero live objects");
        }

        // 3. Template function accepting std::vector reference
        std::cout << "Test 3\n";
        {
            auto p = parseCode(
                "#include <vector>\n"
                "template<typename T>\n"
                "T sum_container(const std::vector<T>& v) {\n"
                "    T total = 0;\n"
                "    for (T x : v) total += x;\n"
                "    return total;\n"
                "}\n"
                "int main() {\n"
                "    std::vector<int> nums = {1, 2, 3, 4};\n"
                "    return sum_container(nums);\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 10, "template function accepting container reference");
            check(r.liveObjectCount() == 0, "sum container zero live objects");
        }

        // 4. Vector resize with user-defined class construction and destruction
        std::cout << "Test 4\n";
        {
            auto p = parseCode(
                "#include <vector>\n"
                "int alive = 0;\n"
                "class Widget {\n"
                "public:\n"
                "    int id;\n"
                "    Widget() : id(1) { alive++; }\n"
                "    Widget(const Widget& o) : id(o.id) { alive++; }\n"
                "    ~Widget() { alive--; }\n"
                "};\n"
                "int main() {\n"
                "    {\n"
                "        std::vector<Widget> v;\n"
                "        v.resize(4);\n"
                "        if (alive != 4) return 1;\n"
                "        v.resize(2);\n"
                "        if (alive != 2) return 2;\n"
                "    }\n"
                "    return alive;\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 0, "vector resize ctor/dtor lifecycle");
            check(r.liveObjectCount() == 0, "vector resize zero live objects");
        }

        // 5. Vector clear and copy assignment lifecycle
        std::cout << "Test 5\n";
        {
            auto p = parseCode(
                "#include <vector>\n"
                "int alive = 0;\n"
                "class Box {\n"
                "public:\n"
                "    int val;\n"
                "    Box() : val(0) { alive++; }\n"
                "    Box(int v) : val(v) { alive++; }\n"
                "    Box(const Box& o) : val(o.val) { alive++; }\n"
                "    ~Box() { alive--; }\n"
                "};\n"
                "int main() {\n"
                "    {\n"
                "        std::vector<Box> v1;\n"
                "        Box b1(10);\n"
                "        v1.push_back(b1);\n"
                "        std::vector<Box> v2;\n"
                "        Box b2(20);\n"
                "        Box b3(30);\n"
                "        v2.push_back(b2);\n"
                "        v2.push_back(b3);\n"
                "        v2 = v1;\n"
                "        if (v2.size() != 1) return 10;\n"
                "        v1.clear();\n"
                "        if (v1.size() != 0) return 20;\n"
                "    }\n"
                "    return alive;\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 0, "vector clear and copy assign lifecycle");
            check(r.liveObjectCount() == 0, "vector clear/assign zero live objects");
        }

        // 6. Dereferencing one-past-the-end rejected
        std::cout << "Test 6\n";
        {
            auto p = parseCode(
                "#include <vector>\n"
                "int main() {\n"
                "    std::vector<int> v = {10, 20};\n"
                "    auto it = v.end();\n"
                "    return *it;\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            bool caught = false;
            try {
                r.run(p);
            } catch (const Error&) {
                caught = true;
            }
            check(caught, "dereference one-past-the-end rejected");
        }

        // 7. Dangling pointer after pop_back rejected
        std::cout << "Test 7\n";
        {
            auto p = parseCode(
                "#include <vector>\n"
                "int main() {\n"
                "    std::vector<int> v = {10, 20};\n"
                "    int* p = &v.back();\n"
                "    v.pop_back();\n"
                "    return *p;\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            bool caught = false;
            try {
                r.run(p);
            } catch (const Error&) {
                caught = true;
            }
            check(caught, "dereference after pop_back rejected");
        }

        // 8. Stale iterator after clear rejected
        std::cout << "Test 8\n";
        {
            auto p = parseCode(
                "#include <vector>\n"
                "int main() {\n"
                "    std::vector<int> v = {10, 20};\n"
                "    auto it = v.begin();\n"
                "    v.clear();\n"
                "    return *it;\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            bool caught = false;
            try {
                r.run(p);
            } catch (const Error&) {
                caught = true;
            }
            check(caught, "dereference after clear rejected");
        }

        // 9. std::move value category audit
        std::cout << "Test 9\n";
        {
            auto p = parseCode(
                "#include <utility>\n"
                "int main() {\n"
                "    int x = 42;\n"
                "    int y = std::move(x);\n"
                "    return y;\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 42, "std::move on scalar value");
            check(r.liveObjectCount() == 0, "move scalar zero live objects");
        }

        // 10. Copying class template objects
        std::cout << "Test 10\n";
        {
            auto p = parseCode(
                "template<typename T>\n"
                "class PairHolder {\n"
                "public:\n"
                "    T a;\n"
                "    T b;\n"
                "    PairHolder(T x, T y) : a(x), b(y) {}\n"
                "    T sum() const { return a + b; }\n"
                "};\n"
                "int main() {\n"
                "    PairHolder<int> p1(10, 20);\n"
                "    PairHolder<int> p2 = p1;\n"
                "    p2.a = 30;\n"
                "    return p1.sum() * 100 + p2.sum();\n"
                "}\n"
            );
            Analyzer{}.check(p);
            std::ostringstream out;
            Interpreter r(out);
            check(r.run(p) == 3050, "class template copy independence");
            check(r.liveObjectCount() == 0, "class template copy zero live objects");
        }

    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
        return 1;
    }

    std::cout << "All " << total << " RC1.1 unit tests passed!\n";
    return 0;
}
