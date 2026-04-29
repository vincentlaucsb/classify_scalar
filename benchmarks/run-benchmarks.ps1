param(
    [string]$BuildDir = "$PSScriptRoot\out\build",
    [string]$Config = "Release",
    [string]$Generator = "Ninja",
    [string]$VsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
    [string]$MinTime = "0.5s",
    [string[]]$BenchmarkArgs = @()
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

function Find-BenchmarkExe {
    param([string]$Name)

    $candidates = @(
        (Join-Path $BuildDir "$Name.exe"),
        (Join-Path $BuildDir $Config "$Name.exe"),
        (Join-Path $BuildDir $Name),
        (Join-Path $BuildDir $Config $Name)
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Could not find $Name executable under $BuildDir"
}

Write-Host "Repository: $repoRoot"
$defaultArgs = @("--benchmark_min_time=$MinTime")
$allArgs = $defaultArgs + $BenchmarkArgs

foreach ($name in @("classify_scalar_benchmarks", "classify_scalar_benchmarks_fallback")) {
    $exe = Find-BenchmarkExe $name
    Write-Host ""
    Write-Host "Running: $exe $allArgs"
    & $exe @allArgs
}
