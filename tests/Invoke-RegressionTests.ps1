param
(
	[Parameter(Mandatory = $true)]
	[string]$Executable
)

$ErrorActionPreference = "Stop"
$Executable = (Resolve-Path -LiteralPath $Executable).Path
$TestRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$Failures = 0

function Invoke-SuccessCase
{
	param
	(
		[string]$Name,
		[string]$Script,
		[string[]]$ExpectedLines
	)

	$PreviousErrorAction = $ErrorActionPreference
	$ErrorActionPreference = "Continue"
	try
	{
		$Output = @(& $Executable (Join-Path $TestRoot $Script) 2>&1)
		$ExitCode = $LASTEXITCODE
	}
	finally
	{
		$ErrorActionPreference = $PreviousErrorAction
	}
	$Lines = @($Output | ForEach-Object { $_.ToString().TrimEnd() })

	if($ExitCode -ne 0)
	{
		Write-Host "[FAIL] $Name exited with $ExitCode"
		$script:Failures++
		return
	}

	foreach($ExpectedLine in $ExpectedLines)
	{
		if($Lines -notcontains $ExpectedLine)
		{
			Write-Host "[FAIL] $Name did not emit '$ExpectedLine'"
			$script:Failures++
			return
		}
	}

	Write-Host "[PASS] $Name"
}

function Invoke-FailureCase
{
	param
	(
		[string]$Name,
		[string]$Script,
		[string]$ExpectedText
	)

	$PreviousErrorAction = $ErrorActionPreference
	$ErrorActionPreference = "Continue"
	try
	{
		$Output = @(& $Executable (Join-Path $TestRoot $Script) 2>&1)
		$ExitCode = $LASTEXITCODE
	}
	finally
	{
		$ErrorActionPreference = $PreviousErrorAction
	}
	$Text = $Output -join [Environment]::NewLine

	if($ExitCode -eq 0 -or $Text -notlike "*$ExpectedText*")
	{
		Write-Host "[FAIL] $Name did not fail as expected"
		$script:Failures++
		return
	}

	Write-Host "[PASS] $Name"
}

Invoke-SuccessCase "string equality" "TestStringEquality.sun" @("PASS_EQUAL", "PASS_NOTEQUAL")
Invoke-SuccessCase "division tokenization" "TestDivision.sun" @("4")
Invoke-SuccessCase "return and array lifetime" "TestReturnAndArray.sun" @("PASS_RETURN", "PASS_ARRAY")
Invoke-SuccessCase "valid array bounds" "TestBounds.sun" @("2")
Invoke-SuccessCase "reserved language features" "TestLanguageFeatures.sun" @("PASS_EMPTY_BRANCH", "5")
Invoke-SuccessCase "operator precedence" "TestPrecedence.sun" @("1", "true")
Invoke-SuccessCase "forward function call" "TestForwardCall.sun" @("12")
Invoke-SuccessCase "multidimensional arrays" "TestMultidimensional.sun" @("3")
Invoke-SuccessCase "tracing garbage collection" "TestGarbageCollection.sun" @("1", "PASS_GC")
Invoke-SuccessCase "string escapes" "TestStringEscapes.sun" @("PASS", "ESCAPE")
Invoke-FailureCase "invalid array bounds" "TestBoundsFailure.sun" "Index out of bounds"
Invoke-FailureCase "unterminated string" "TestCompileFailure.sun" "Unterminated string literal"
Invoke-FailureCase "oversized array index" "TestHugeIndex.sun" "Array index is too large"
Invoke-FailureCase "oversized numeric literal" "TestHugeNumber.sun" "outside the supported range"

$MissingPath = Join-Path $TestRoot "Missing.sun"
$PreviousErrorAction = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try
{
	$MissingOutput = @(& $Executable $MissingPath 2>&1)
	$MissingExitCode = $LASTEXITCODE
}
finally
{
	$ErrorActionPreference = $PreviousErrorAction
}

if($MissingExitCode -eq 0)
{
	Write-Host "[FAIL] missing source file returned success"
	$Failures++
}
else
{
	Write-Host "[PASS] missing source file"
}

if($Failures -ne 0)
{
	Write-Host "$Failures regression test(s) failed"
	exit 1
}

Write-Host "All regression tests passed"
exit 0
