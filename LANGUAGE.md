# Solstice Script Language

Solstice Script preserves the syntax of the original student project while
making previously reserved constructs operational.

## Values

- `nil`
- booleans: `true`, `false`
- double-precision numbers
- strings with `\n`, `\r`, `\t`, `\"`, and `\\` escapes
- zero-based arrays

Only `nil`, `false`, and numeric zero are false in conditions. This preserves
the original language's use of zero as false.

## Variables

Assignments create globals at the top level and function locals inside
functions:

```lua
score = 10

function add(value)
    result = value + score
    return result
end
```

Use `local` for explicit block-scoped variables:

```lua
do
    local message = "scoped"
    printline(message)
end
```

## Functions

```lua
function factorial(value)
    if value == 0 then
        return 1
    end

    return value * factorial(value - 1)
end
```

Functions may be called before their declarations. Calls require the declared
number of arguments.

## Arrays

```lua
values = {1, 2, 3}
values[1] = 10
printline(values[1])

matrix = {{1, 2}, {3, 4}}
printline(matrix[1][0])
```

Array assignment grows an array and fills intermediate elements with `nil`.
Array reads outside the current size are runtime errors. Indexes must be
non-negative whole numbers.

## Control flow

```lua
if condition then
    printline("yes")
elseif otherCondition then
    printline("maybe")
else
    printline("no")
end

while count < 10 do
    count = count + 1
end

for index = 1, 10, 2 do
    printline(index)
end

repeat
    count = count - 1
until count == 0
```

The third numeric `for` expression is optional and defaults to `1`.

## Operators

From lowest to highest precedence:

1. assignment: `=`
2. logical or: `or`
3. logical and: `and`
4. comparison: `==`, `~=`, `<`, `>`, `<=`, `>=`
5. concatenation: `..`
6. addition and subtraction: `+`, `-`
7. multiplication and division: `*`, `/`
8. unary operators: `-`, `not`
9. indexing and calls

Logical operators short circuit and return boolean values.

## Comments

Both forms are accepted:

```lua
-- Lua-style comment
// C++-style comment
```

## Built-in functions

| Function | Description |
|---|---|
| `print(value)` | Writes a value without a trailing newline |
| `printline(value)` | Writes a value followed by a newline |
| `length(array)` | Returns the current array size |

## Diagnostics

Compile and runtime errors contain the source path, line, and column whenever
the location is known:

```text
examples\errors\array-out-of-bounds.sun:4:11: error: Array index out of bounds
```

Use `--dump-tokens`, `--dump-ast`, and `--dump-bytecode` to inspect each
compiler stage.

## Complete examples

The [example catalog](examples/README.md) contains runnable demonstrations of
every major language feature, including recursion, sorting, matrices, lexical
scope, forward calls, cyclic arrays, and garbage collection.
