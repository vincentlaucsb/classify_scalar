param(
    [string]$BuildDir = "",
    [string]$Config = "Release",
    [string]$Generator = "Ninja",
    [string]$VsDevCmd = "",
    [string]$MinTime = "0.5s",
    [string[]]$BenchmarkArgs = @()
)

$ErrorActionPreference = "Stop"

$sourceDir = $PSScriptRoot
$repoRoot = Resolve-Path "$PSScriptRoot\.."
$script:ResolvedVsDevCmd = $null

function Find-CmdExe {
    $candidates = @(
        $env:ComSpec,
        (Join-Path $env:SystemRoot "System32\cmd.exe"),
        "C:\Windows\System32\cmd.exe"
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return $candidate
        }
    }

    throw "Could not find cmd.exe. Set ComSpec or pass a usable Windows system path."
}

function Get-WindowsSystemPath {
    $windowsRoot = if ($env:SystemRoot) { $env:SystemRoot } else { "C:\Windows" }
    return @(
        (Join-Path $windowsRoot "System32"),
        $windowsRoot,
        (Join-Path $windowsRoot "System32\Wbem"),
        (Join-Path $windowsRoot "System32\WindowsPowerShell\v1.0")
    ) -join ";"
}

function Find-VsDevCmd {
    if ($VsDevCmd -and (Test-Path $VsDevCmd)) {
        return $VsDevCmd
    }

    $candidates = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Invoke-NativeBuildCommand {
    param([string]$Command)

    if ($env:OS -eq "Windows_NT" -and $script:ResolvedVsDevCmd) {
        $cmdExe = Find-CmdExe
        $systemPath = Get-WindowsSystemPath
        & $cmdExe /d /s /c "set `"Path=`" && set `"PATH=$systemPath`" && call `"$script:ResolvedVsDevCmd`" -arch=x64 -host_arch=x64 && $Command"
    } else {
        Invoke-Expression $Command
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Command"
    }
}

$script:ResolvedVsDevCmd = Find-VsDevCmd
if (-not $BuildDir) {
    $BuildDir = if ($script:ResolvedVsDevCmd -like "*\Microsoft Visual Studio\18\*") {
        "$PSScriptRoot\out\build-vs2026"
    } else {
        "$PSScriptRoot\out\build"
    }
}

if ($script:ResolvedVsDevCmd) {
    Write-Host "Using Visual Studio environment: $script:ResolvedVsDevCmd"
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

$exe = Find-BenchmarkExe "classify_scalar_benchmarks"
Write-Host ""
Write-Host "Running: $exe $allArgs"
& $exe @allArgs
