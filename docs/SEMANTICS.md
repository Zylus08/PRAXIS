# Semantic rules

Parsing creates an AST, then `Analyzer` resolves names and annotates expression type/value category without executing user code. Runtime consumes that checked tree. Function signatures and object declarations become visible in source order. A function sees globals declared before its definition, never caller locals. Self-recursion works; mutually recursive functions require forward declarations. No implicit function declarations exist.

Objects have identity independent of copied values. Reading an lvalue materializes its current value; assignment targets and references retain the storage handle. Prefix increment and assignment produce lvalues; postfix increment produces the old copied value. Constness is checked on the expression, so a const reference to a mutable object is read-only through that alias. References require matching base type and a live lvalue; no temporary lifetime extension is claimed.

## Arithmetic and conversion

The target model is Windows LLP64 for the types implemented: char is signed 8-bit, int is signed 32-bit, long long is signed 64-bit. Bool represents zero/one. Float/double are binary32/binary64; the host build is required to have these widths. Bool and char promote to int. The usual arithmetic result rank is double, float, long long, int. Unsuffixed decimal integers choose int then long long; only LL/ll suffixes are accepted. Float results round back to binary32 after each operation.

Signed arithmetic overflow, division/remainder by zero, non-finite floating results, and numeric conversions outside the destination range produce runtime diagnostics. Integer operations are guarded before host arithmetic, including long-long minimum divided by minus one. Integer division truncates toward zero. Floating-to-integral conversion truncates toward zero before checking range. Numeric-to-bool tests nonzero. Out-of-range integral narrowing is deliberately rejected at runtime rather than emulating C++20 modular narrowing; this excludes some otherwise-defined C++ programs. Non-finite floating behavior is also a deliberate restricted-subset policy.

Array list initialization conservatively rejects decreasing numeric rank, even when a C++ constant-expression narrowing exception would permit it. Assignment initialization permits representable numeric conversions. Scalar list initialization is not implemented.

## Sequencing and initialization

Logical operators short-circuit. Assignment evaluates its right side before its left side, consistent with C++20. Stream insertion evaluates left before right. Other operands and function arguments use left-to-right evaluation, one permitted choice for supported well-defined programs. **Programs relying on unsequenced conflicting accesses or on a particular unspecified evaluation order are outside the subset.** General undefined-behavior detection and alias-effect analysis are not implemented; the interpreter is not a UB sanitizer.

Global objects undergo zero initialization and supported constant initialization before dynamic initializers execute in source order. The constant evaluator handles numeric literals/operators, short-circuiting, prior const integral objects, numeric array lists and direct global reference binding. General constexpr evaluation is not implemented; initialization-order-sensitive programs beyond these forms are outside the subset. Local scalar objects without initializers are indeterminate; reading one is diagnosed at runtime. Default strings are empty. Initialized arrays zero-fill missing elements; uninitialized automatic array elements remain indeterminate. A value parameter is a distinct object; a reference parameter aliases the argument object.

A non-void function other than main must have a structurally provable return: a direct return in a block or both if/else branches returning. This conservative analysis can reject valid functions whose termination requires reasoning about loops. Main falling off the end returns zero. Void functions can fall off the end or use `return;`.

Runtime diagnostics are safety policies of Praxis, not assertions that a conforming C++ compiler must diagnose those cases. Differential fixtures contain only well-defined programs in the overlap.
