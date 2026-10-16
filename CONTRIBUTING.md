# Contributing

Solstice Script is primarily an educational and archival project, but focused
bug fixes, tests, documentation, and small language improvements are welcome.

## Requirements

- Visual Studio 2022
- Desktop development with C++
- MSVC v143
- Windows 10 or later SDK
- PowerShell 5.1 or PowerShell 7

## Build

```powershell
msbuild SolsticeScriptCompiler.sln `
    /t:Build `
    /p:Configuration=Debug `
    /p:Platform=x64
```

## Test

```powershell
.\x64\Debug\SolsticeScriptTests.exe

.\tests\Invoke-RegressionTests.ps1 `
    -Executable .\x64\Debug\SolsticeScriptCompiler.exe

.\examples\Invoke-Examples.ps1 `
    -Executable .\x64\Debug\SolsticeScriptCompiler.exe
```

Run the same commands for Release before submitting runtime changes.

## Change guidelines

- Preserve existing valid Solstice scripts unless a breaking change is
  explicitly documented.
- Add a regression test for every behavior fix.
- Keep source-located diagnostics for compile and runtime failures.
- Use the VM allocation API for garbage-collected values crossing the native
  boundary.
- Avoid optimizing value representation or instruction dispatch without
  profiling evidence.
