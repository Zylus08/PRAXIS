# Constructor, Destructor & RAII Lifetime Semantics

**Milestone:** Project PRAXIS — Milestone M2  
**Target Specifications:** Sections 4, 5, 11, 13  

---

## 1. Constructor Initialization Pipeline

In Project PRAXIS, object construction follows strict C++ semantic ordering, ensuring robust object initialization and lifetime guarantees.

### 1.1 Constructor Synthesis and Overload Selection
1. **Implicit Default Constructor:**
   - If a class declares no constructors, an implicit default constructor is synthesized (`Function::Special::DefaultConstructor`).
   - If any constructor is declared by the user, the implicit default constructor is suppressed. Attempting default construction without a user default constructor produces a compile-time error:
     ```text
     semantic: no viable overload (argument count, type or template deduction mismatch)
     ```
2. **Constructor Overload Resolution:**
   - Constructors participate in standard overload resolution via `Analyzer::constructor()`. Candidates are ranked based on argument types and conversion ranks.

### 1.2 Member Initializer List Ordering
A fundamental rule of C++ (ISO/IEC 14882) is that class non-static data members are initialized in **declaration order**, regardless of their order in the constructor's member initializer list.

Praxis strictly enforces this rule:
1. During semantic analysis (`Analyzer::checkFunction`), the constructor's member initializer list is inspected.
2. Initializers are checked for duplicate entries or references to nonexistent members.
3. The initializers are reorganized into a canonical vector `f->initializers` ordered strictly by `Class::fields` (declaration order).
4. Unspecified members with in-class default initializers receive cloned expressions of their defaults.
5. Unspecified scalar members without default initializers remain uninitialized (or zero-initialized if in static/global scope or value-initialized via `$zero`).

#### Verification Proof: `examples/m2/member_initializer_order.cpp`
```cpp
#include <iostream>
#include <string>
struct A { A(int n){std::cout<<n;} ~A(){std::cout<<"1";} };
struct B { B(int n){std::cout<<n;} ~B(){std::cout<<"2";} };
struct C {
    A a;
    B b;
    C() : b(2), a(1) { std::cout << "PQ"; } // Initializer list lists b before a
};
int main() { C c; }
```
- **Execution Output:** `12PQ21`
  - Construction: `A(1)` prints `1`, then `B(2)` prints `2`, then constructor body prints `PQ`.
  - Destruction: `B` is destroyed first printing `2`, then `A` is destroyed printing `1`.
  - Proves declaration order construction and reverse declaration order destruction.

### 1.3 Partial Construction Failure Handling
If an error or exception occurs while constructing an object (for example, division by zero or resource exhaustion inside a member constructor or constructor body):
1. The incompletely initialized object must **never** become accessible in user scope.
2. Sub-objects (members) that have already completed construction must be destroyed in reverse construction order.
3. Sub-objects that were not yet constructed must **not** have their destructors invoked.

Praxis implements this via `Interpreter::construct` and `Interpreter::initializeMembers`:
```cpp
void Interpreter::construct(const O& o, const std::string& id, const std::vector<Evaluation>& args) {
    Timer timer(profile.constructorNanoseconds);
    try {
        call(functions.at(id), args, o);
        o->initialized = true;
        o->complete = true;
        ++profile.constructions;
    } catch (...) {
        auto primary = std::current_exception();
        try { destroy(o); } catch (...) {}
        std::rethrow_exception(primary);
    }
}
```
In `Interpreter::destroy(const O& o)`:
```cpp
for (auto it = o->members.rbegin(); it != o->members.rend(); ++it)
    try { destroy(*it); } catch (...) {}
```
Because `o->members` records members as they successfully finish initialization, only completed members are cleaned up. `o->complete` remains `false`, and the object is never inserted into the lexical scope.

Verified by:
- `tests/m2/fixtures/constructor_failure.cpp`
- `tests/m2/fixtures/partial_member_failure.cpp`
- `tests/unit/m2.cpp` (`badCtor` check: `check(failing.liveObjectCount() == 0 && failing.statistics().destructors == 1)`).

---

## 2. Destructors and Deterministic RAII Lifetime

Praxis implements complete, deterministic Resource Acquisition Is Initialization (RAII) semantics.

### 2.1 Reverse Construction-Order Destruction
In any scope, objects are registered in `Scope::owned` in the exact order they are constructed. Upon scope exit, `Interpreter::cleanup(Scope& scope)` iterates through `scope.owned` in **reverse order** (`rbegin()` to `rend()`):
1. User-defined destructor (if present) is called on the object.
2. The object's member variables are destroyed in reverse declaration order.
3. Array elements are destroyed in reverse index order.
4. Storage is marked dead (`alive = false`, `complete = false`).
5. Live object tally is decremented.

### 2.2 Scope Exit Boundaries

#### 1. Normal Block Exit
When execution leaves a block `{ ... }`, all objects allocated within that block are destroyed immediately in reverse construction order.

#### 2. Early Exit via `return`
When a `return` statement executes:
1. The return expression is evaluated.
   - If returning a class object by value, an lvalue expression is copy-constructed into a temporary return slot, or a prvalue temporary is directly detached.
2. All scopes on the active function frame are popped and cleaned up in reverse nesting order.
3. All local objects are destroyed before control returns to the caller.
4. The return value is safely delivered to the caller.

Verified by `examples/m2/raii_return.cpp`:
```cpp
#include <iostream>
#include <string>
struct T { int id; T(int n):id(n){std::cout<<"C"<<id;} ~T(){std::cout<<"D"<<id;} };
int f() { T a(1); { T b(2); return 7; } }
int main() { std::cout << f(); }
```
- **Output:** `C1C2D2D17`
- Proves both inner `b` and outer `a` destructors run deterministically on `return`, in reverse order of construction, prior to caller receiving result `7`.

#### 3. Early Exit via `break` and `continue`
Inside loops (`for`, `while`, `do-while`), loop bodies execute inside nested lexical scopes.
- Executing `break` or `continue` cleans up all objects constructed in the current loop iteration before control branches to the loop exit or loop step/condition.
- No local objects leak across iterations.

Verified by `examples/m2/raii_loop_control.cpp`:
```cpp
#include <iostream>
#include <string>
struct G { int i; G(int v):i(v){std::cout<<"C"<<i;} ~G(){std::cout<<"D"<<i;} };
int main() {
    for (int i=0; i<3; ++i) {
        G g(i);
        if (i==1) break;
    }
}
```
- **Output:** `C0D0C1D1`
- Iteration 0: `C0` constructed, `D0` destroyed on reaching end of loop body.
- Iteration 1: `C1` constructed, `D1` destroyed immediately when `break` executes.

### 2.3 Temporary Object Lifetime
Temporary objects created in expressions (e.g. `C(1).get()`) are tracked in `Interpreter::temporaries`.
- At the completion of each full-expression evaluation (`Interpreter::discard`, `Interpreter::test`, `Interpreter::execute`), `cleanupTemporaries(mark)` destroys all temporary objects created during that statement.
- If a temporary is returned or moved to an initialized object via prvalue copy elision, `detachTemporary()` transfers ownership, preventing premature destruction.

### 2.4 Prevention of Double Destruction
Double destruction is mathematically impossible in Praxis due to state invariants:
1. `o->destroying`: A boolean re-entrancy guard set at the entrance of `destroy(o)`.
2. `o->alive = false`: Tombstones the object. Any subsequent call to `destroy(o)` immediately returns if `!o->alive`.
3. `Scope::owned.clear()`: Empties ownership lists immediately upon beginning cleanup, preventing multiple cleanup passes from processing the same pointer.

---

## 3. Reference vs Object Ownership Invariants

1. **References Do Not Own Storage:**
   - A reference variable aliases an existing `Object*`.
   - The alias is added to `Scope::names`, but **never** to `Scope::owned`.
   - Exiting a scope that contains a reference alias does not destroy the referent.
2. **Access After Lifetime Expiration:**
   - If a reference outlives its referent (e.g. via static reference binding or manual pointer aliasing), the referent's `alive` flag will be `false`.
   - Every read and write to an object via `Interpreter::object()` asserts `requireAlive(o, loc)`.
   - Use-after-destruction produces an immediate runtime diagnostic:
     ```text
     runtime: object lifetime has ended
     ```
3. **No Hidden Garbage Collection:**
   - Destruction is 100% deterministic and bound to source-level lexical scopes.
