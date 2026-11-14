# M1 architecture

```text
SourceManager -> Preprocessor + Lexer -> token stream -> Parser -> AST
                                                        |
                                                Analyzer (scopes/types)
                                                        |
                                              Interpreter (tree walk)
                                               /                 \
                                     explicit object storage    native bindings
```

- `common/source.hpp`, `frontend/source.cpp`: source loading, locations, phase-tagged diagnostics.
- `frontend/lexer.*`: independent lexical scanner and token stream. Tokens distinguish literal contents from grammatical spelling.
- `frontend/preprocessor.*`: token-based object macros and includes. Expansion locations point to invocation sites; included source retains file/line locations. Tokens carry native-header visibility at their original source position.
- `frontend/parser.*`: recursive-descent declarations/statements and precedence-climbing expressions. No external parsing library.
- `ast/ast.hpp`, `ast/type.cpp`: owned AST nodes, type descriptors, expression value categories, function/declaration nodes. AST ownership is independent of runtime frames.
- `semantics/analyzer.*`: source-order symbols, lexical scopes, signatures, type compatibility, const/lvalue checks, loop context, return validation. `check` terminates after this stage.
- `runtime/value.*`: typed values, initialized/live object records, checked arithmetic and conversions.
- `runtime/interpreter.*`: expression evaluation, control propagation, frames, lookup, reference binding, global initialization and scope cleanup.
- `native/bindings.*`: explicitly bridged output and string operations.
- `cli/main.cpp`: commands and incremental submission history.
- `tests/unit`: direct frontend assertions; `tests/integration`: compiler-free fixture verification; `tests/differential`: external reference compilation.

## M2/M3 extension points

Class declarations need new AST nodes, class symbols, member lookup and layout metadata. `Object` and explicit scope expiration provide a place to add constructors and reverse-order destruction, but M1 does not implement those features. Copy initialization currently goes through value conversion; class special-member selection must be added there deliberately.

Function signatures currently form a single-entry map per name. Overload sets and conversion ranking should replace this map before adding overload resolution. Namespace scopes should extend symbol lookup instead of adding more parser hardcoding. The current `std::string`/`std::cout` spellings are documented native special cases, not a namespace implementation.

Template syntax, dependent types, instantiation caches, and substitution diagnostics require a separate semantic layer over retained declaration ASTs. The tree-walking backend can remain the first executor for instantiated functions. These are design directions, not implemented placeholder APIs.

No source under `include/` or `src/` starts a process, links a compiler API, loads a compiler frontend, or generates machine code. Formatter/toolchain usage during development is unrelated to interpreted program execution.
