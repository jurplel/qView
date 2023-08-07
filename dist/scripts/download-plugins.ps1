#!/usr/bin/env pwsh

# This script will download binary plugins from the kimageformats-binaries repository using Github's API.

$pluginNames = "qtapng", "kimageformats"

$qtVersion = ((qmake --version -split '\n')[1] -split ' ')[3]
Write-Host "Detected Qt Version $qtVersion"

# Update these to change which artifacts to download!
$avifBuildNum = 51
$apngBuildNum = 66
$kimageformatsBuildNum = 107

# Qt version availability and runner names are assumed.
if ($IsWindows) {
    $imageName = "windows-2019"
} elseif ($IsMacOS) {
    $imageName = "macos-latest"
} else {
    $imageName = "ubuntu-20.04"
}

$binaryBaseUrl = "https://github.com/jurplel/kimageformats-binaries/releases/download/cont"

if ($pluginNames.count -eq 0) {
    Write-Host "the pluginNames array is empty."
}

foreach ($pluginName in $pluginNames) {
    $arch = If (-not $env:arch -or $env:arch -eq '') { "" } Else { "-$env:arch" }
    $artifactName = "$pluginName-$imageName-$qtVersion$arch.zip"
    $downloadUrl = "$binaryBaseUrl/$artifactName"

    Write-Host "Downloading $downloadUrl"
    Invoke-WebRequest -URI $downloadUrl -OutFile $artifactName
    Expand-Archive $artifactName -DestinationPath $pluginName
    Remove-Item $artifactName
}


if ($IsWindows) {
    $out_frm = "bin/"
    $out_imf = "bin/imageformats"
} elseif ($IsMacOS) {
    $out_frm = "bin/qView.app/Contents/Frameworks"
    $out_imf = "bin/qView.app/Contents/PlugIns/imageformats"
} else {
    $out_frm = "bin/appdir/usr/lib"
    $out_imf = "bin/appdir/usr/plugins/imageformats"
}

mkdir -p "$out_frm" -ErrorAction SilentlyContinue
mkdir -p "$out_imf" -ErrorAction SilentlyContinue

# Copy QtApng
if ($pluginNames -contains 'qtapng') {
    if ($IsWindows) {
        cp qtapng/QtApng/plugins/imageformats/qapng.dll "$out_imf/"
    } elseif ($IsMacOS) {
        cp qtapng/QtApng/plugins/imageformats/libqapng.dylib "$out_imf/"
    } else {
        cp qtapng/QtApng/plugins/imageformats/libqapng.so "$out_imf/"
    }
}

if ($pluginNames -contains 'kimageformats') {
    if ($IsWindows) {
        cp kimageformats/kimageformats/output/kimg_*.dll "$out_imf/"
        cp kimageformats/kimageformats/output/zlib1.dll "$out_frm/"
        cp kimageformats/kimageformats/output/KF5Archive.dll "$out_frm/"
    } elseif ($IsMacOS) {
        cp kimageformats/kimageformats/output/*.so "$out_imf/"
        cp kimageformats/kimageformats/output/libKF5Archive.5.dylib "$out_frm/"
    } else {
        cp kimageformats/kimageformats/output/kimg_*.so "$out_imf/"
        cp kimageformats/kimageformats/output/libKF5Archive.so.5 "$out_frm/"
    }
}
