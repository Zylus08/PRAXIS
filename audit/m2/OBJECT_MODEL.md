# Praxis Object Model, Storage Architecture & Copy Semantics

**Milestone:** Project PRAXIS — Milestone M2  
**Target Specifications:** Sections 3, 6, 9  

---

## 1. Class and Struct Type Representation

In Project PRAXIS M2, classes and structs are first-class types represented in the typed AST and runtime environment.

### 1.1 Type Metadata (`Type`)
The AST type descriptor `Type` is extended to support user-defined composite types:
```cpp
enum class Base {
    Void, Bool, Char, Int, Long, Float, Double, String, Text, Stream, Endl,
    Record,     // User-defined class or struct instance
    Parameter   // Template type parameter (e.g., T)
};

struct Type {
    Base base = Base::Void;
    bool constant = false;
    bool reference = false;
    int extent = 0;              // Fixed array bound (0 if scalar/record)
    std::string name;            // Type name / class identifier
    std::vector<Type> arguments; // Template type arguments (e.g., <int>)
};
```
When `base == Base::Record`, `name` holds the class name, and `arguments` contains any template specialization arguments. The fully resolved canonical name is produced by `typeName(Type)`, yielding identifiers such as `Box<int>` or `Point`.

### 1.2 Class AST Definition (`Class`)
Class declarations are parsed into `Class` nodes:
```cpp
struct Class {
    Token name;                                     // Class identifier
    std::string key;                                // Canonical mangled key
    std::vector<Decl> fields;                       // Member variables (declaration order)
    std::vector<std::shared_ptr<Function>> methods; // Member functions & constructors
    std::vector<std::string> templateParameters;    // Template parameter names
    Type type;                                      // Record type descriptor
    std::string copyConstructor;                    // ID of copy constructor
    std::string destructor;                         // ID of destructor (if defined)
    bool assignable = true;                         // False if class contains const fields
    size_t nestingDepth = 1;                        // Nested class member depth
};
```

---

## 2. Class vs Struct & Access Control

Praxis implements standard C++ access control rules:
1. **Default Access:**
   - In a `class`, default access for members preceding any access label is `private`.
   - In a `struct`, default access for members preceding any access label is `public`.
2. **Access Specifiers:**
   - `public:` and `private:` labels alter subsequent member access within the class body.
3. **Static Access Enforcement:**
   - Member access via `.` or implicit member access within methods is statically checked by `Analyzer::checkAccess` and `Analyzer::memberExpression`.
   - Accessing a `private` member variable or method from outside the defining class produces a compile-time semantic error:
     ```text
     semantic: private data member is inaccessible
     semantic: private member is inaccessible
     ```
   - In-class member accesses (where `currentClass == c->key`) are permitted full access to private members.

---

## 3. Runtime Object Storage and Identity

### 3.1 Object Structure (`Object`)
Every runtime lvalue storage location in Praxis is represented by a heap-allocated `Object` instance:
```cpp
struct Object {
    Type type;                                      // Concrete instantiated type
    Value value;                                    // Scalar data payload (for primitives)
    bool initialized = false;                       // Has construction finished?
    bool alive = true;                              // Is storage active?
    bool complete = false;                          // Are all members fully initialized?
    bool destroying = false;                        // Re-entry guard during destruction
    bool zeroInitialize = false;                    // Static/global zero-init semantics
    Location location;                              // Allocation origin location
    std::vector<std::shared_ptr<Object>> elements;  // Array elements
    std::map<std::string, std::shared_ptr<Object>> fields; // Fast lookup by name
    std::vector<std::shared_ptr<Object>> members;   // Members in declaration order
    std::shared_ptr<Class> definition;              // Class schema metadata
};
```

### 3.2 Member Storage Layout
When an object of class type is constructed:
- `fields` provides instantaneous name-based member access (`o->fields["n"]`).
- `members` stores member `Object` pointers strictly in **declaration order**.
- This dual representation guarantees $O(1)$ member access while enforcing exact reverse-order member destruction.

### 3.3 Object Identity and Reference Semantics
- **Identity:** Each `allocate()` call creates a unique `std::shared_ptr<Object>`. Objects have distinct addresses.
- **Reference Binding:**
  ```cpp
  int a = 10;
  int& r = a;
  ```
  In Praxis, the declaration of reference `r` inserts `a`'s `std::shared_ptr<Object>` into `current().names["r"]`.
  Crucially, `r` is **not** appended to `current().owned`. Consequently:
  1. The alias does not own the referent.
  2. Destroying the scope containing the alias does not destroy or decrement the referent's lifetime.
  3. Mutating `r` directly mutates `a`'s storage.
  4. If the referent's lifetime ends (e.g. referent was stack-allocated in a popped scope), subsequent reads via `r` detect `o->alive == false` and fail with:
     ```text
     runtime: object lifetime has ended
     ```

---

## 4. Member Access & Implicit `this`

### 4.1 Member Access via `.`
Evaluating `expr.member`:
1. `Analyzer::memberExpression` evaluates the base expression and verifies it is a class type (`Base::Record`).
2. Checks access control permissions against the calling context.
3. Propagates value category: if the base expression is an lvalue, the member access expression is an lvalue (`e->lvalue = true`).
4. Propagates `const` qualification: if the base object is `const`, the member expression is treated as `const`, preventing mutation.

### 4.2 Implicit `this` Resolution
Inside a member function, unqualified references to member variables or member functions are resolved as implicit member operations:
- AST nodes have `implicitMember = true`.
- At runtime, `eval()` retrieves `self = frames.back()->self`.
- Validates `requireAlive(self, loc)`.
- Accesses `self->fields.at(e->op)`.

### 4.3 `const` Member Functions
- Member functions marked with `const` guarantee that the implicit object cannot be modified.
- Inside a const member function, `constMethod == true`.
- Attempting to mutate a data member or call a non-const member function triggers a semantic error:
  ```text
  semantic: cannot mutate const object or member
  semantic: no viable overload (argument count, type or template deduction mismatch)
  ```
- Calling a non-const method on a const object is rejected during overload candidate viability filtering.

---

## 5. Copy Construction, Copy Assignment & Lifetime Management

### 5.1 Distinction Between Copy Construction and Copy Assignment
Praxis maintains a strict, fundamental distinction between copy construction and copy assignment:
- **Copy Construction:** Initializes a brand-new, uninitialized `Object` instance from an existing valid `Object`.
- **Copy Assignment:** Mutates an already-existing, fully initialized destination `Object` using values from another initialized `Object`.

### 5.2 Synthesized and User-Defined Copy Constructors
1. **Implicit Synthesis:** If no user-defined copy constructor is declared, Praxis synthesizes a default copy constructor (`Function::Special::CopyConstructor`).
   - For each field in declaration order:
     - If the field is a scalar, its value is read and assigned to a newly allocated field object.
     - If the field is a class object, it is recursively copy-constructed via `copyObject()`.
2. **User-Defined Copy Constructor:**
   - Supported when declared with signature `ClassName(const ClassName& other)`.
   - The user-defined body executes, initializing members according to its member initializer list or defaults.

### 5.3 Copy Assignment Semantics (`assignObject`)
Copy assignment (`a = b` where `a` and `b` are class objects):
```cpp
void Interpreter::assignObject(const O& destination, const O& source, const Location& l) {
    requireAlive(destination, l);
    requireAlive(source, l);
    if (destination == source) return; // Self-assignment safety
    if (!destination->complete || !source->complete)
        fail(l, "runtime", "assignment involving incomplete object");
    if (destination->definition->key != source->definition->key)
        fail(l, "runtime", "class assignment type mismatch");
    
    ++profile.assignments;
    for (auto& d : destination->definition->fields) {
        auto a = destination->fields.at(d.name.text);
        auto b = source->fields.at(d.name.text);
        if (d.type.base == Base::Record)
            assignObject(a, b, l);
        else {
            a->value = read({{}, b}, l);
            a->initialized = true;
        }
    }
}
```
- **Self-Assignment Guard:** If `destination == source`, the operation is a no-op, preventing corruption.
- **Const Invalidation:** If any field in the class is declared `const`, `Class::assignable` is set to `false`, and assignment is rejected at compile time:
  ```text
  semantic: cannot mutate const object or member
  ```

### 5.4 Prvalue Copy Elision
In direct initialization:
```cpp
C c = C(10);
```
Praxis identifies that the initializer is a temporary prvalue of the identical class type (`!init->lvalue && a == b`). Rather than executing a redundant copy constructor, Praxis elides the copy (`$elide`), directly taking ownership of the temporary storage and avoiding extraneous allocations and destructor calls.
