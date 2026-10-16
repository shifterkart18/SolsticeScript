# Embedding Solstice Script

The `SolsticeScript` static-library project produces `SolsticeScript.lib`.
Include the headers from `SolsticeScriptCompiler` and link the library from an
x64 C++20 application.

## Compile and execute

```cpp
#include "SScript.h"

Solstice::SScript script;
if (!script.Compile("gameplay.sun"))
{
    Log(script.LastError());
    return;
}

if (!script.Execute())
{
    Log(script.LastError());
}
```

`Compile` and `Execute` return `false` for script, native callback, and standard
library failures. `LastError` contains the formatted diagnostic.

## Register a native function

```cpp
script.RegisterFunction(
    "subtract",
    2,
    [](Solstice::SVirtualMachine&, std::span<const Solstice::SValue> arguments)
    {
        return Solstice::SValue::Number(
            arguments[0].AsNumber() - arguments[1].AsNumber());
    });
```

Registration fails for duplicate names or empty callbacks.

Native callback arguments are ordered exactly as written in the script.
Standard exceptions are converted into script runtime errors.

## Create native values

Use the VM passed to the callback for garbage-collected values:

```cpp
script.RegisterFunction(
    "makeMessage",
    0,
    [](Solstice::SVirtualMachine& vm, std::span<const Solstice::SValue>)
    {
        return vm.CreateString("created by C++");
    });
```

`CreateString` and `CreateArray` return externally rooted values. They may be
retained by the host after script execution. Do not call `Execute` recursively
from a native callback; the VM reports reentrant execution as an error.
