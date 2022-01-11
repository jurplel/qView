#!/usr/bin/pwsh

param
(
    $Prefix = "/usr"
)

if ($IsWindows) {
    ci/scripts/vcvars.ps1
}

if ((qmake --version -split '\n')[1][17] == '6') {
    qmake QMAKE_APPLE_DEVICE_ARCHS="x86_64 arm64" $args[0] PREFIX=$Prefix
} else {
    qmake $args[0] PREFIX=$Prefix
}


if ($IsWindows) {
    nmake
} else {
    make
}