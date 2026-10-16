# Solstice Script Examples

Run an individual example:

```powershell
.\x64\Debug\SolsticeScriptCompiler.exe .\examples\factorial.sun
```

Run the complete successful sample suite:

```powershell
.\examples\Invoke-Examples.ps1 `
    -Executable .\x64\Debug\SolsticeScriptCompiler.exe
```

| Sample | Demonstrates |
|---|---|
| `hello-world.sun` | Variables, strings, concatenation, and output |
| `factorial.sun` | Recursion and returns |
| `string-comparison.sun` | String equality and conditional branches |
| `control-flow.sun` | `for`, `while`, `repeat/until`, and short circuiting |
| `arrays-and-matrices.sun` | Dynamic arrays, nil gaps, functions, and matrices |
| `fibonacci.sun` | Recursive computation and numeric iteration |
| `insertion-sort.sun` | Array mutation and a complete sorting algorithm |
| `scopes-and-forward-calls.sun` | Block scope, shadowing, and forward calls |
| `cyclic-arrays.sun` | Cyclic object graphs and tracing garbage collection |
| `strings-and-truthiness.sun` | Escapes and truthiness rules |

The `errors` directory contains scripts that intentionally fail to demonstrate
source-located compile and runtime diagnostics.
