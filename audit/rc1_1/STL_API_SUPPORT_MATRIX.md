# Milestone RC1.1 STL Compatibility Support Matrix

This matrix documents the exact status of the C++ Standard Library facilities provided by Praxis v0.1.0.

Status definitions:
- **Implemented:** Fully supported within the Praxis execution model and differentially verified against MSVC.
- **Implemented with Restrictions:** Working with documented architectural or subset limitations.
- **Recognized but Incomplete:** Syntactically parsed or bound as a no-op / placeholder.
- **Unsupported:** Rejected with clear semantic diagnostics if attempted.

---

## 1. `<vector>`

| API / Feature | Status | Notes |
|---|---|---|
| `std::vector<T>()` | **Implemented** | Default constructor creates empty vector. |
| `std::vector<T>(size)` | **Implemented** | Default-constructs `size` elements of type `T`. |
| `std::vector<T>(size, val)` | **Implemented** | Initializes `size` elements copied from `val`. |
| `std::vector<T>{...}` | **Implemented** | Initializer list construction supported for scalars, records, and strings. |
| `std::vector<T>(const vector&)` | **Implemented** | Deep copy of all elements. |
| `operator=` | **Implemented** | Cleans up existing elements (calling destructors) and deep-copies source. |
| `push_back(const T&)` | **Implemented** | Appends element; deep-copies class objects; bumps generation counter. |
| `pop_back()` | **Implemented** | Destroys last element; throws error if called on empty vector. |
| `clear()` | **Implemented** | Destroys all elements in reverse order; bumps generation counter. |
| `size()` | **Implemented** | Returns current element count as `int64_t`. |
| `empty()` | **Implemented** | Returns `bool` whether vector has 0 elements. |
| `resize(new_size)` | **Implemented** | Downward resize destroys excess elements; upward resize default-constructs new elements. |
| `resize(new_size, val)` | **Implemented** | Upward resize initializes new elements by copying `val`. |
| `reserve(capacity)` | **Implemented with Restrictions** | Recognized and syntax-checked; abstract storage manages memory without host reallocations. |
| `front()` | **Implemented** | Returns lvalue reference to first element; error on empty vector. |
| `back()` | **Implemented** | Returns lvalue reference to last element; error on empty vector. |
| `at(index)` | **Implemented** | Bounds-checked element access; throws error if out of range. |
| `operator[](index)` | **Implemented** | Indexing supported; checked for bounds safety in Praxis runtime. |
| `begin()` / `end()` | **Implemented** | Returns abstract pointer handles; `end()` represents one-past-the-end handle. |
| Range-based for loop | **Implemented** | `for (T x : v)` and `for (auto x : v)` fully supported. |
| `emplace_back` | **Unsupported** | Variadic templates not supported. |
| `insert` / `erase` | **Unsupported** | Mid-vector insertions/deletions not implemented in v0.1.0. |
| Custom Allocator | **Unsupported** | Interpreter manages object storage abstractly. |

---

## 2. `<string>`

| API / Feature | Status | Notes |
|---|---|---|
| `std::string()` | **Implemented** | Default constructor creates empty string. |
| `std::string(const char*)` | **Implemented** | Construction from string literal. |
| `std::string(const string&)` | **Implemented** | Copy constructor. |
| `operator=` | **Implemented** | Copy assignment from string or literal. |
| `operator+` / `+=` | **Implemented** | Concatenation with strings and char literals. |
| `size()` / `length()` | **Implemented** | Returns length of string. |
| `empty()` | **Implemented** | Returns boolean emptiness. |
| `at(index)` / `operator[](index)` | **Implemented** | Returns character at index; bounds checked. |
| `substr(pos, count)` | **Implemented** | Extracts substring with optional count. |
| Range-based for loop | **Implemented** | Iterates over string characters. |
| `find` / `replace` | **Unsupported** | String search/replace methods not in v0.1.0 subset. |
| Iterators (`begin`/`end`) | **Unsupported** | String iterators not implemented; use indexing or range-for. |

---

## 3. `<utility>`

| API / Feature | Status | Notes |
|---|---|---|
| `std::pair<T1, T2>` | **Implemented** | Direct construction and brace initialization. |
| `std::make_pair(a, b)` | **Implemented** | Template helper creating pairs. |
| `.first`, `.second` | **Implemented** | Member access to pair elements. |
| `std::swap(a, b)` | **Implemented** | In-place value/vector element swap. |
| `std::move(a)` | **Implemented with Restrictions** | Rvalue expression cast; falls back to copy construction. |
| `std::forward` | **Unsupported** | Universal references / perfect forwarding unsupported. |

---

## 4. `<algorithm>`

| API / Feature | Status | Notes |
|---|---|---|
| `std::min(a, b)` | **Implemented** | Scalar and numeric minimum. |
| `std::max(a, b)` | **Implemented** | Scalar and numeric maximum. |
| `std::sort(begin, end)` | **Implemented** | Stable sort over random-access iterator handles. |
| `std::reverse(begin, end)` | **Implemented** | In-place element reversal over iterator handles. |
| `std::find(begin, end, val)` | **Implemented** | Linear search returning iterator handle to found element or `end()`. |
| `std::count(begin, end, val)` | **Implemented** | Counts matching elements in range. |
| Custom Predicates | **Unsupported** | Algorithms use default `<` and `==` operators. |
| Parallel / Execution Policies | **Unsupported** | C++17 execution policies unsupported. |

---

## 5. `<array>`

| API / Feature | Status | Notes |
|---|---|---|
| `std::array<T, N>` | **Implemented** | Fixed-size aggregate container. |
| `operator[](index)` | **Implemented** | Bounds-checked element access. |
| `size()` | **Implemented** | Returns compile-time extent `N`. |
| Range-based for loop | **Implemented** | Iterates over array elements. |
| `fill` / `swap` / tuple API | **Unsupported** | Not implemented in v0.1.0. |

---

## 6. `<iostream>`

| API / Feature | Status | Notes |
|---|---|---|
| `std::cout` | **Implemented** | Standard output stream. |
| `operator<<` | **Implemented** | Output for ints, longs, doubles, chars, strings, pointers. |
| `std::endl` | **Implemented** | Newline and flush. |
| `std::cin` | **Unsupported** | Standard input stream not supported in batch interpretation. |
| `std::cerr` | **Unsupported** | Diagnostic stream; interpreter emits runtime errors directly to stderr. |
| Stream Manipulators (`hex`, `setw`)| **Unsupported** | Formatting manipulators not implemented. |
