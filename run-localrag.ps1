param(
    [int]$Port = 4190,
    [switch]$LanAccess
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$frontendRoot = Join-Path $projectRoot 'frontend'
$binaryPath = Join-Path $projectRoot 'build-desktop-final\localrag.exe'

if (-not (Get-Command node -ErrorAction SilentlyContinue)) {
    throw 'Node.js 18 or newer is required.'
}

if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
    throw 'npm was not found. Check that Node.js is on PATH.'
}

if (-not (Test-Path (Join-Path $frontendRoot 'package.json'))) {
    throw "Frontend directory not found: $frontendRoot"
}

if (-not (Test-Path (Join-Path $frontendRoot 'node_modules'))) {
    Write-Host 'Installing frontend dependencies...' -ForegroundColor Cyan
    Push-Location $frontendRoot
    try {
        & npm.cmd ci
        if ($LASTEXITCODE -ne 0) { throw 'Frontend dependency installation failed.' }
    } finally {
        Pop-Location
    }
}

if (-not (Test-Path $binaryPath)) {
    $buildDirectory = Join-Path $projectRoot 'build-desktop-final'
    if (-not (Test-Path $buildDirectory)) {
        throw "Local service binary and build directory are missing: $buildDirectory"
    }
    Write-Host 'Building localrag...' -ForegroundColor Cyan
    Push-Location $projectRoot
    try {
        & cmake --build $buildDirectory --target localrag -j 4
        if ($LASTEXITCODE -ne 0) { throw 'localrag build failed.' }
    } finally {
        Pop-Location
    }
}

if ($LanAccess) {
    $listenAddress = '0.0.0.0'
    $routedInterfaces = @(
        Get-NetRoute -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty InterfaceIndex -Unique
    )
    $lanAddresses = @(
        Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
            Where-Object {
                $_.InterfaceIndex -in $routedInterfaces -and
                $_.AddressState -eq 'Preferred' -and
                $_.IPAddress -ne '127.0.0.1' -and
                $_.IPAddress -notlike '169.254.*'
            } |
            Select-Object -ExpandProperty IPAddress -Unique
    )
    if ($lanAddresses.Count -eq 0) {
        throw 'No LAN IPv4 address found. Connect to a network or start without -LanAccess.'
    }
    Write-Warning 'LAN access exposes this local API to devices on your network. Do not use on a public or untrusted network.'
    foreach ($address in $lanAddresses) {
        Write-Host "LocalRAG LAN URL: http://${address}:$Port/" -ForegroundColor Green
    }
} else {
    $listenAddress = '127.0.0.1'
    Write-Host "LocalRAG web app (this PC only): http://127.0.0.1:$Port/" -ForegroundColor Green
}
Write-Host 'Keep this window open. Press Ctrl+C to stop.' -ForegroundColor DarkGray

Push-Location $frontendRoot
try {
    & npm.cmd run dev -- --host $listenAddress --port $Port --strictPort
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
