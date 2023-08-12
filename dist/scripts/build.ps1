#!/usr/bin/env pwsh

param
(
    $Prefix = "/usr"
)

if ($IsWindows) {
    dist/scripts/vcvars.ps1
}

if ((qmake --version -split '\n')[1][17] -eq '6') {
    qmake QMAKE_APPLE_DEVICE_ARCHS="x86_64 arm64" $args[0] PREFIX=$Prefix DEFINES+="$env:nightlyDefines"
} else {
    qmake $args[0] PREFIX=$Prefix DEFINES+="$env:nightlyDefines"
}


if ($IsWindows) {
    nmake
} else {
    make
}
