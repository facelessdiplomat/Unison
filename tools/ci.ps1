#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot

function Invoke-Checked
{
    param([Parameter(Mandatory)] [string] $Activity, [Parameter(Mandatory)] [scriptblock] $Command)

    Write-Host "ci: $Activity"

    & $Command

    if ($LASTEXITCODE -ne 0)
    {
        throw "$Activity failed with exit code $LASTEXITCODE"
    }
}

$env:CTEST_PARALLEL_LEVEL = [Math]::Max(1, [int][Math]::Floor([Environment]::ProcessorCount / 2))

function Invoke-Workflow
{
    param([Parameter(Mandatory)] [string] $Preset)

    Invoke-Checked "workflow $Preset" { cmake --workflow --preset $Preset }
}

function Test-TrackedFormatting
{
    $trackedSources = & git ls-files '*.cpp' '*.hpp'

    if (-not $trackedSources)
    {
        throw 'no tracked C++ sources to check'
    }

    Invoke-Checked 'clang-format' { & $env:UNISON_CLANG_FORMAT --dry-run --Werror @trackedSources }
}

& "$PSScriptRoot/env.ps1"

Push-Location $repositoryRoot

try
{
    Invoke-Workflow msvc-debug
    Invoke-Workflow msvc-release
    Test-TrackedFormatting
    Write-Host 'ci: ok'
}
catch
{
    Write-Host "ci: $_"
    exit 1
}
finally
{
    Pop-Location
}
