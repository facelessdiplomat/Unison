#Requires -Version 5.1

function Find-VisualStudioInstallation
{
    Set-StrictMode -Version Latest
    $ErrorActionPreference = 'Stop'

    $vsWherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vsWherePath))
    {
        throw "vswhere.exe was not found at $vsWherePath"
    }

    $installationPath = & $vsWherePath -latest -products '*' `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath

    if ([string]::IsNullOrWhiteSpace($installationPath))
    {
        throw 'No Visual Studio installation with the x64 C++ toolset was found'
    }

    return $installationPath
}

function Import-DeveloperEnvironment
{
    param([Parameter(Mandatory)] [string] $InstallationPath)

    Set-StrictMode -Version Latest
    $ErrorActionPreference = 'Stop'

    if ($env:VSCMD_ARG_TGT_ARCH -eq 'x64')
    {
        return
    }

    $vcvarsPath = Join-Path $InstallationPath 'VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path -LiteralPath $vcvarsPath))
    {
        throw "vcvars64.bat was not found at $vcvarsPath"
    }

    $variables = & $env:ComSpec /s /c "`"$vcvarsPath`" >nul 2>&1 && set"
    if ($LASTEXITCODE -ne 0)
    {
        throw "vcvars64.bat failed with exit code $LASTEXITCODE"
    }

    foreach ($variable in $variables)
    {
        $separator = $variable.IndexOf('=')
        if ($separator -gt 0)
        {
            [Environment]::SetEnvironmentVariable($variable.Substring(0, $separator), $variable.Substring($separator + 1))
        }
    }
}

function Export-BundledToolPaths
{
    param([Parameter(Mandatory)] [string] $InstallationPath)

    Set-StrictMode -Version Latest
    $ErrorActionPreference = 'Stop'

    $relativePaths = [ordered] @{
        UNISON_CMAKE        = 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
        UNISON_NINJA        = 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
        UNISON_CLANG_FORMAT = 'VC\Tools\Llvm\x64\bin\clang-format.exe'
    }

    foreach ($name in $relativePaths.Keys)
    {
        $toolPath = Join-Path $InstallationPath $relativePaths[$name]
        if (-not (Test-Path -LiteralPath $toolPath))
        {
            throw "$name was not found at $toolPath"
        }

        [Environment]::SetEnvironmentVariable($name, $toolPath)
    }
}

$visualStudioPath = Find-VisualStudioInstallation
Import-DeveloperEnvironment -InstallationPath $visualStudioPath
Export-BundledToolPaths -InstallationPath $visualStudioPath

Write-Host "Unison build environment: MSVC $env:VCToolsVersion x64 from $visualStudioPath"
