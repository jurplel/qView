#!/usr/bin/env pwsh

param
(
    [switch]$CI,
    $Prefix = "/usr"
)

if ($IsWindows) {
    dist/scripts/vcvars.ps1
} elseif ($IsMacOS) {
    if (-not $CI -and $qtVersion -lt [version]'6.5.3') {
        # Workaround for QTBUG-117484
        sudo xcode-select --switch /Applications/Xcode_14.3.1.app
    }
}

# Prepare CMake arguments
$cmakeArgs = @(
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_INSTALL_PREFIX=$Prefix"
)

if ($env:nightlyDefines) {
    $cmakeArgs += "-D$($env:nightlyDefines)"
}

if ($IsMacOS -and $env:buildArch -eq 'Universal') {
    $cmakeArgs += "-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64"
} elseif ($IsWindows) {
    # Workaround for https://developercommunity.visualstudio.com/t/10664660
    $cmakeArgs += '-DCMAKE_CXX_FLAGS=-D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR'
    if ($env:buildArch -eq 'X86') {
        $cmakeArgs += "-A Win32"
    } elseif ($env:buildArch -eq 'Arm64') {
        $cmakeArgs += "-A ARM64"
    }
}

# Create a build directory, configure, and build
New-Item -ItemType Directory -Force -Path build
Push-Location build
try {
    cmake $cmakeArgs ..
    cmake --build . --config Release --parallel
} finally {
    Pop-Location
}

# Copy artifact to bin directory for deployment scripts
New-Item -ItemType Directory -Force -Path bin
if ($IsWindows) {
    # MSVC generator might put executables in a config-specific subdirectory
    $exePath = "build/Release/qView.exe"
    if (-not (Test-Path $exePath)) {
        $exePath = "build/qView.exe"
    }
    Copy-Item -Path $exePath -Destination "bin/qView.exe" -Force
} elseif ($IsMacOS) {
    Copy-Item -Path "build/qView.app" -Destination "bin/qView.app" -Recurse -Force
} else {
    Copy-Item -Path "build/qview" -Destination "bin/qview" -Force
}
