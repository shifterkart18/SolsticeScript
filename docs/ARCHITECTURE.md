# Architecture

Solstice Script uses a conventional compiler and virtual-machine pipeline.

## Compilation

1. `SLexicalAnalyzer` reads a UTF-8-compatible byte stream and emits tokens
   carrying source file, line, and column information.
2. `SParser` performs recursive-descent parsing and resolves global and local
   variable slots.
3. `SNode` owns the abstract syntax tree through `std::unique_ptr`.
4. `SCodeGenerator` emits typed `Instruction` values rather than packing
   operands into a byte array.

Functions are generated before top-level code, which permits forward calls.
Call instructions are resolved after all function offsets are known.

## Execution

`SVirtualMachine` maintains:

- a value stack
- global variable slots
- explicit call frames
- an instruction pointer
- a tracing garbage collector

Function frame metadata does not share storage with script values. Returning a
value copies it before the frame is removed.

## Values and garbage collection

`SValue` uses `std::variant` for nil, booleans, numbers, strings, and arrays.
Strings and arrays are allocated by `GarbageCollector`.

Collection marks:

- VM stack values
- globals
- arrays reachable from those values
- values retained through the native extension boundary

This permits nested and cyclic arrays without reference-count cycles.

## Native boundary

Native functions are registered as `std::function` callbacks receiving
`std::span<const SValue>`. Argument values are externally rooted for the
duration of the callback and remain rooted if copied into host storage.

Native-created strings and arrays are rooted until the host releases them.
Values returned to VM-owned storage have their external root removed.

The VM rejects reentrant execution. Standard exceptions raised by native
callbacks are translated into source-located script runtime errors.
