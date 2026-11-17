$ErrorActionPreference = 'Stop'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$requiredVersion = '[18.12,19.0)'
$requiredComponents = @('Microsoft.Component.MSBuild', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64')

$ready = & $vswhere -latest -version $requiredVersion -products '*' -requires $requiredComponents -property installationPath
if ($LASTEXITCODE -ne 0)
{
    throw "Visual Studio discovery failed with exit code $LASTEXITCODE."
}

if (-not $ready)
{
    $installation = & $vswhere -latest -version '[18.0,19.0)' -products '*' -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $installation)
    {
        throw 'A complete Visual Studio 2026 installation is required on the hosted runner.'
    }

    $installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
    $arguments = "update --installPath `"$installation`" --channelUri https://aka.ms/vs/stable/channel --quiet --norestart"
    $process = Start-Process -FilePath $installer -ArgumentList $arguments -Wait -PassThru
    if ($process.ExitCode -ne 0)
    {
        throw "Visual Studio update failed with exit code $($process.ExitCode)."
    }

    $ready = & $vswhere -latest -version $requiredVersion -products '*' -requires $requiredComponents -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $ready)
    {
        throw 'Visual Studio 2026 18.12 or later with the required C++ tools was not found after the update.'
    }
}

Write-Output "Using Visual Studio 2026: $ready"
