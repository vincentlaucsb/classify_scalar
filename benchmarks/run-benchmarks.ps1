param(
    [string]$BuildDir = "$PSScriptRoot\out\build",
    [string]$Config = "Release",
    [string]$Generator = "Ninja",
    [string]$VsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
    [string[]]$BenchmarkArgs = @("--benchmark_min_time=0.2s")
)

$ErrorActionPreference = "Stop"

$sourceDir = $PSScriptRoot
$repoRoot = Resolve-Path "$PSScriptRoot\.."

function Invoke-NativeBuildCommand {
    param([string]$Command)

    if ($env:OS -eq "Windows_NT" -and (Test-Path $VsDevCmd)) {
        & cmd.exe /d /s /c "call `"$VsDevCmd`" -arch=x64 -host_arch=x64 && $Command"
    } else {
        Invoke-Expression $Command
    }
}

Invoke-NativeBuildCommand "cmake -S `"$sourceDir`" -B `"$BuildDir`" -G `"$Generator`" -DCMAKE_BUILD_TYPE=$Config"
Invoke-NativeBuildCommand "cmake --build `"$BuildDir`" --config $Config"

$exe = Join-Path $BuildDir "classify_scalar_benchmarks.exe"
if (-not (Test-Path $exe)) {
    $exe = Join-Path $BuildDir $Config "classify_scalar_benchmarks.exe"
}
if (-not (Test-Path $exe)) {
    $exe = Join-Path $BuildDir "classify_scalar_benchmarks"
}
if (-not (Test-Path $exe)) {
    throw "Could not find classify_scalar_benchmarks executable under $BuildDir"
}

Write-Host "Repository: $repoRoot"
Write-Host "Running: $exe $BenchmarkArgs"
& $exe @BenchmarkArgs
