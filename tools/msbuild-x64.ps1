param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("Build", "Clean", "Rebuild")]
    [string]$Target = "Build"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $repoRoot "CGP_VS2022.sln"
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

$msbuild = $null
if (Test-Path $vswhere) {
    $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\amd64\MSBuild.exe" | Select-Object -First 1
}

if (-not $msbuild) {
    $msbuildCommand = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
    if ($msbuildCommand) {
        $msbuild = $msbuildCommand.Source
    }
}

if (-not $msbuild) {
    throw "MSBuild.exe was not found. Install Visual Studio 2022 with the 'Desktop development with C++' workload."
}

& $msbuild $solution "/t:$Target" "/p:Configuration=$Configuration" "/p:Platform=x64" /m
exit $LASTEXITCODE
