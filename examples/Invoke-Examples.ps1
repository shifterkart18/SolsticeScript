param
(
	[Parameter(Mandatory = $true)]
	[string]$Executable
)

$ErrorActionPreference = "Stop"
$Executable = (Resolve-Path -LiteralPath $Executable).Path
$ExampleRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

$Examples =
@(
	"hello-world.sun",
	"factorial.sun",
	"string-comparison.sun",
	"control-flow.sun",
	"arrays-and-matrices.sun",
	"fibonacci.sun",
	"insertion-sort.sun",
	"scopes-and-forward-calls.sun",
	"cyclic-arrays.sun",
	"strings-and-truthiness.sun"
)

foreach($Example in $Examples)
{
	Write-Host ""
	Write-Host "=== $Example ==="
	& $Executable (Join-Path $ExampleRoot $Example)
	if($LASTEXITCODE -ne 0)
	{
		throw "Example '$Example' failed with exit code $LASTEXITCODE"
	}
}

Write-Host ""
Write-Host "All examples completed successfully"
