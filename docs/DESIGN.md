# Design Notes

## Goals

Solstice Script is a compact, understandable scripting-language implementation
that demonstrates the complete path from source text to execution:

```text
source -> tokens -> AST -> typed instructions -> virtual machine
```

The modernized edition prioritizes correctness, readable implementation, safe
embedding, and useful diagnostics over maximum execution speed.

## Compatibility

The language preserves valid scripts from the original project where practical.
Numeric zero remains false to retain the original runtime behavior, even though
this differs from Lua.

## Deliberate boundaries

Solstice Script currently provides arrays rather than Lua-style tables. It also
does not yet implement closures, modules, coroutines, generic iteration, or hot
reload. These are natural future directions, but they would require explicit
language and embedding contracts rather than incremental syntax additions.

## Runtime representation

- `SValue` uses `std::variant`.
- Strings and arrays are tracing-GC objects.
- Native-retained values register external roots.
- Bytecode is represented by typed `Instruction` structures.
- Function execution uses explicit call frames.
- Native callbacks use ordered `std::span<const SValue>` arguments.

This representation is intentionally straightforward. More compact encodings,
NaN boxing, generational collection, or JIT compilation should only be
considered after representative profiling.
