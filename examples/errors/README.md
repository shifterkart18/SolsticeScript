# Diagnostic Samples

These scripts intentionally fail and demonstrate source-located diagnostics.

```powershell
.\x64\Debug\SolsticeScriptCompiler.exe `
    .\examples\errors\array-out-of-bounds.sun
```

`array-out-of-bounds.sun` produces a runtime error with its file, line, and
column. `syntax-error.sun` produces a compile error for the missing `end`.
