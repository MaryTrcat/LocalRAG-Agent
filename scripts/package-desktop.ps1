param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [string]$OutputDir = 'E:\codex\LocalRAGDesktop'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $projectRoot 'build-desktop-final'
$exe = Join-Path $buildDir 'localrag_desktop.exe'
$windeployqt = 'E:\QT\6.11.0\mingw_64\bin\windeployqt.exe'
$mingwBin = 'E:\cstudy\x86_64-13.2.0-release-win32-seh-ucrt-rt_v11-rev1\mingw64\bin'

if (-not (Test-Path $exe)) {
    & (Join-Path $PSScriptRoot 'build-desktop.ps1') -Configuration $Configuration
}
if (-not (Test-Path $exe)) { throw "找不到桌面程序：$exe" }

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $OutputDir 'localrag_desktop.exe') -Force
& $windeployqt --release --no-translations --no-compiler-runtime (Join-Path $OutputDir 'localrag_desktop.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt 运行时部署失败' }

foreach ($runtime in @('libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'libwinpthread-1.dll')) {
    $runtimePath = Join-Path $mingwBin $runtime
    if (Test-Path $runtimePath) {
        Copy-Item -LiteralPath $runtimePath -Destination (Join-Path $OutputDir $runtime) -Force
    }
}

$configDir = Join-Path $OutputDir 'config'
New-Item -ItemType Directory -Force -Path $configDir | Out-Null
$configPath = Join-Path $configDir 'localrag.json'
if (-not (Test-Path $configPath)) {
    Copy-Item -LiteralPath (Join-Path $projectRoot 'config\localrag.example.json') -Destination $configPath
}
New-Item -ItemType Directory -Force -Path (Join-Path $OutputDir 'data') | Out-Null
Write-Host "桌面程序已打包到 $OutputDir"
