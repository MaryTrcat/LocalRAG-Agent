param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $projectRoot 'build-desktop-final'
$qtDir = 'E:\QT\6.11.0\mingw_64\lib\cmake\Qt6'
$projectCompiler = 'E:\cstudy\x86_64-13.2.0-release-win32-seh-ucrt-rt_v11-rev1\mingw64\bin\g++.exe'

cmake -S $projectRoot -B $buildDir -G Ninja `
    "-DCMAKE_PREFIX_PATH=E:/QT/6.11.0/mingw_64" `
    "-DQt6_DIR=$qtDir" `
    "-DCMAKE_CXX_COMPILER=$projectCompiler"
if ($LASTEXITCODE -ne 0) { throw 'CMake 配置失败' }

cmake --build $buildDir --config $Configuration --parallel 4
if ($LASTEXITCODE -ne 0) { throw '桌面程序构建失败' }

ctest --test-dir $buildDir --output-on-failure
if ($LASTEXITCODE -ne 0) { throw '测试失败' }

Write-Host "LocalRAG Desktop $Configuration 构建完成。"
