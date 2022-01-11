#!/usr/bin/pwsh

param
(
    $Prefix = "/usr"
)

if ($IsWindows) {
    ci/scripts/vcvars.ps1
}

qmake QMAKE_APPLE_DEVICE_ARCHS="x86_64 arm64" $args[0] PREFIX=$Prefix

if ($IsWindows) {
    nmake
} else {
    make
}