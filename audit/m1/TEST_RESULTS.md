# Test results — PASS

Verified implementation commit: `f33b72a9b58f36396fcb638a0f49f8a1003799f5`. Test manifests contain SHA-256 hashes matching the delivered binaries.

| Suite | Debug | Release |
|---|---:|---:|
| C++ frontend assertions | 15/15 | 15/15 |
| Integration cases | 244/244 | 244/244 |
| Adversarial/environment cases | 13/13 | 13/13 |
| Differential programs (separate suite) | 134/134 | 134/134 |

The first three suites provide **272 checks per configuration**. We do not multiply counts for `check`, execution, and deterministic re-execution of the same case. CLI demonstrations separately passed 14/14 (12 examples plus help/version). These overlap some fixtures and are not added to that 272 total.

## Integration categories

| Category | Debug passed | Release passed |
|---|---:|---:|
| arithmetic | 38 | 38 |
| assignment | 11 | 11 |
| conversion | 8 | 8 |
| control | 5 | 5 |
| loops | 10 | 10 |
| scope | 8 | 8 |
| array | 6 | 6 |
| reference | 6 | 6 |
| string | 11 | 11 |
| native | 2 | 2 |
| lexer | 9 | 9 |
| function | 15 | 15 |
| cli | 7 | 7 |
| preprocess | 8 | 8 |
| semantic_rejection | 47 | 47 |
| syntax_rejection | 10 | 10 |
| lex_rejection | 9 | 9 |
| preprocess_rejection | 6 | 6 |
| runtime_rejection | 19 | 19 |
| repl | 9 | 9 |

Unit checks exercise token classification, spellings, escapes, locations, maximal munch, EOF, precedence tree shape and type annotation. Integration checks run `check` without execution, validate phase-tagged failures, compare known output/exit status, and re-run for determinism. Nine REPL scenarios cover declarations, functions, aliases, arrays, macros, multiline input, static rollback, runtime recovery and reset.

Adversarial checks cover deeply nested parentheses/blocks/unary expressions/calls, extreme literals, unterminated comments/escapes, embedded NUL input, recursive macros, invalid for syntax, reference-return rejection, and execution with compiler tools absent from PATH. All diagnosed cleanly; no crashes or timeouts occurred in this set.

Debug uses MSVC `/Od /Z7 /RTC1`; Release uses `/O2`; both use `/W4 /permissive-` and C++20. Both CTest targets passed in both builds. Build and CTest logs are preserved alongside JSON result records. No sanitizer run or broader conformance certification is claimed.
