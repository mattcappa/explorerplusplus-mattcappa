param(
    [switch]$Run
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $vswhere))
{
    throw 'Install Visual Studio 2022 or Build Tools for Visual Studio 2022 with the Desktop development with C++ workload and a Windows SDK, then press F5 again. See BUILDING.md.'
}

$msbuild = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\Current\Bin\MSBuild.exe'

if ($LASTEXITCODE -ne 0)
{
    throw "Visual Studio discovery failed with exit code $LASTEXITCODE."
}

if (-not $msbuild)
{
    throw 'No Visual Studio installation with MSBuild and the C++ x64 build tools was found. Install the Desktop development with C++ workload and a Windows SDK. See BUILDING.md.'
}

$vcpkgRoot = Join-Path $repositoryRoot 'Explorer++\ThirdParty\vcpkg'

if (-not (Test-Path -LiteralPath (Join-Path $vcpkgRoot 'scripts\buildsystems\msbuild\vcpkg.props')))
{
    throw 'The vcpkg submodule is missing. Run git submodule update --init --recursive from the repository root, then run .\Explorer++\ThirdParty\vcpkg\bootstrap-vcpkg.bat.'
}

if (-not (Test-Path -LiteralPath (Join-Path $vcpkgRoot 'vcpkg.exe')))
{
    throw 'vcpkg has not been bootstrapped. Run .\Explorer++\ThirdParty\vcpkg\bootstrap-vcpkg.bat from the repository root.'
}

$solution = Join-Path $repositoryRoot 'Explorer++\Explorer++.sln'
& $msbuild $solution '/t:Explorer++' '/p:Configuration=Debug' '/p:Platform=x64' '/m'
$buildExitCode = $LASTEXITCODE
if ($buildExitCode -ne 0 -or -not $Run)
{
    exit $buildExitCode
}

$outputDirectory = Join-Path $repositoryRoot 'Explorer++\Explorer++\x64\Debug'
$executable = Join-Path $outputDirectory 'Explorer++.exe'
$app = Start-Process -FilePath $executable -WorkingDirectory $outputDirectory -PassThru -Wait
exit $app.ExitCode
