# Execution and lifetime model

`Value` contains a semantic type plus an int64, double, or owned string payload. A variant is representation only: explicit conversion and operator routines determine target-language behavior. Integer widths are not inferred from the variant's int64 capacity.

`Object` stores declared type, initialization status, lifetime status, current value, and optional element objects. References are handles to existing objects. Owning scopes track their own allocations separately from their name map; destroying an alias does not destroy its referent.

Each user call creates a `Frame` containing a stack of lexical `Scope` objects. Lookup searches the current frame then globals. It never searches caller frames. Parameters and the function body share the function's outer scope, so redeclaring a parameter there is rejected. Nested statements get lexical scopes, including unbraced loop bodies. A for-initializer scope lasts through condition, body, and increment.

Scope exit explicitly marks owned objects and array elements dead. Normal completion, return, break, continue, and interpreter exceptions all run cleanup. The host RAII guards implement the scope-exit mechanism; the interpreted lifetime rules are the explicit alive/owned records. M1 has no user destructors or class-object destruction semantics. Future destructor dispatch belongs at the object-expiration boundary.

Statement execution returns a `Flow` value (normal, return, break, continue). Loops consume break/continue and propagate return. For continue executes increment; do/while continue still tests its condition. Return copies its value before cleanup.

Execution is bounded: 10,000,000 steps per run/REPL submission, 128 call frames, 256 active expression evaluations. Parsing limits nesting to 256 and expression-tree height to 128; preprocessing limits include depth to 64, macro depth to 100, and individual expansion output to 1,000,000 tokens. These are implementation limits, not C++ language rules. Inputs still allocate memory and execution is not a security sandbox.

File execution has source diagnostics on stderr, program output on stdout, and main's integer return value as process status. Diagnostic failures return 1; CLI misuse and caught internal/resource exceptions return 2. REPL prints a banner and continues after recoverable submission errors.
