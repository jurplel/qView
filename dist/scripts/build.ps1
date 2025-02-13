#!/usr/bin/env pwsh

param
(
    $Prefix = "/usr"
)

if ($IsWindows) {
    dist/scripts/vcvars.ps1
} elseif ($IsMacOS) {
    if ($qtVersion -lt [version]'6.5.3') {
        # Workaround for QTBUG-117484
        sudo xcode-select --switch /Applications/Xcode_14.3.1.app
    }
}

if ($IsMacOS -and $env:buildArch -eq 'Universal') {
    $argDeviceArchs = 'QMAKE_APPLE_DEVICE_ARCHS=x86_64 arm64'
} elseif ($IsWindows) {
    # Workaround for https://developercommunity.visualstudio.com/t/10664660
    $argVcrMutexWorkaround = 'DEFINES+=_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR'
}
qmake $args[0] PREFIX=$Prefix DEFINES+="$env:nightlyDefines" $argVcrMutexWorkaround $argDeviceArchs

if ($IsWindows) {
    nmake
} else {
    make
}
