# OpenBlack local verification script.
#
# Runs, in order:
#   1. Release build
#   2. All unit tests (ctest)
#   3. Land1 headless run
#   4. Log analysis: exit code, minimum log length, stub/error/warning counts
#   5. Screenshot existence check
#
# Exits non-zero on the first failed stage so CI-style usage works.

[CmdletBinding()]
param(
    [string] $BuildDir = 'D:\openblack\cmake-build-presets\msvc-17-vcpkg',
    [string] $GamePath = 'D:\BaW',
    [string] $OutDir = 'D:\openblack\verify-out',
    [int]    $Frames = 100,
    [int]    $ScreenshotFrame = 50,
    [int]    $MinLogLength = 2000,
    [switch] $SkipBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$script:Failures = @()

function Fail([string] $Stage, [string] $Message)
{
    $script:Failures += "[$Stage] $Message"
    Write-Host "FAIL [$Stage] $Message" -ForegroundColor Red
}

function Ok([string] $Stage, [string] $Message)
{
    Write-Host "OK   [$Stage] $Message" -ForegroundColor Green
}

function Run-Step([string] $Stage, [scriptblock] $Action)
{
    Write-Host ''
    Write-Host "=== $Stage ===" -ForegroundColor Cyan
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    try {
        $result = & $Action
        $sw.Stop()
        if ($result -eq $false) {
            Fail $Stage 'returned failure'
            return $false
        }
        Ok $Stage ("completed in {0:n1}s" -f $sw.Elapsed.TotalSeconds)
        return $true
    }
    catch {
        $sw.Stop()
        Fail $Stage $_.Exception.Message
        return $false
    }
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$exe     = Join-Path $BuildDir 'bin\Release\openblack.exe'
$logFile = Join-Path $OutDir 'land1.log'
$shot    = Join-Path $OutDir 'land1.png'

# --- Stage 1: build ---------------------------------------------------------
if ($SkipBuild) {
    Ok 'build' 'skipped by -SkipBuild'
}
elseif (Run-Step 'build' {
        & cmake --build $BuildDir --config Release 2>&1 | Tee-Object -FilePath (Join-Path $OutDir 'build.log')
        if ($LASTEXITCODE -ne 0) { $false } else { $true }
    }) {
    Ok 'build' 'compilation succeeded'
}

if (-not (Test-Path $exe)) {
    Fail 'build' "executable not found: $exe"
    Write-Host ''
    Write-Host 'RESULT: FAILED' -ForegroundColor Red
    $script:Failures | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    exit 1
}

# --- Stage 2: unit tests ----------------------------------------------------
Run-Step 'tests' {
    & ctest --test-dir $BuildDir -C Release --output-on-failure 2>&1 |
        Tee-Object -FilePath (Join-Path $OutDir 'ctest.log')
    if ($LASTEXITCODE -ne 0) { $false } else { $true }
} | Out-Null

# --- Stage 3: Land1 run -----------------------------------------------------
$exitCode = -1
Run-Step 'land1' {
    if (Test-Path $logFile) { Remove-Item $logFile -Force }
    if (Test-Path $shot)    { Remove-Item $shot -Force }

    $args = @(
        '--game-path',      $GamePath,
        '--log-file',       $logFile,
        '--screenshot-path', $shot,
        '--screenshot-frame', "$ScreenshotFrame",
        '--num-frames-to-simulate', "$Frames"
    )
    & $exe @args 2>&1 | Tee-Object -FilePath (Join-Path $OutDir 'land1-stdout.log')
    $script:exitCode = $LASTEXITCODE
    $script:land1Ok = ($script:exitCode -eq 0)
    $script:land1Ok
} | Out-Null

# --- Stage 4: log analysis --------------------------------------------------
$log = if (Test-Path $logFile) { Get-Content $logFile -Raw } else { '' }

if ($exitCode -ne 0) {
    Fail 'land1' "non-zero exit code: $exitCode"
}
else {
    Ok 'land1' 'exit code 0'
}

# A crash can truncate the log, which would make a naive "zero stubs" report
# look clean. Require a plausible log size before trusting the counters.
if ($log.Length -lt $MinLogLength) {
    Fail 'log' ("log too short: {0} chars (expected >= {1}) - run likely crashed" -f $log.Length, $MinLogLength)
}
else {
    Ok 'log' ("length {0} chars" -f $log.Length)
}

$stubCount   = ([regex]::Matches($log, 'not implemented')).Count
$errorCount  = ([regex]::Matches($log, '\[error\]')).Count
$warnCount   = ([regex]::Matches($log, '\[warning\]')).Count

Write-Host ("INFO [log] not implemented: {0}" -f $stubCount)
Write-Host ("INFO [log] errors:          {0}" -f $errorCount)
Write-Host ("INFO [log] warnings:        {0}" -f $warnCount)

if ($stubCount -ne 0) {
    Fail 'log' "unexpected 'not implemented' messages during Land1 load: $stubCount"
}
else {
    Ok 'log' 'no stubs executed'
}

# --- Stage 5: screenshot ----------------------------------------------------
if (-not (Test-Path $shot)) {
    Fail 'screenshot' "not created: $shot"
}
else {
    $size = (Get-Item $shot).Length
    if ($size -lt 10000) {
        Fail 'screenshot' "suspiciously small ($size bytes) - likely blank"
    }
    else {
        Ok 'screenshot' ("{0} bytes at {1}" -f $size, $shot)
    }
}

# --- Summary ----------------------------------------------------------------
Write-Host ''
if ($script:Failures.Count -eq 0) {
    Write-Host 'RESULT: PASSED' -ForegroundColor Green
    exit 0
}
Write-Host 'RESULT: FAILED' -ForegroundColor Red
$script:Failures | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
exit 1
