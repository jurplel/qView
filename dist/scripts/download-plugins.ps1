#!/usr/bin/env pwsh

# This script will download binary plugins from the kimageformats-binaries repository using Github's API.

$pluginNames = "qtapng", "kimageformats"

$qtVersion = ((qmake --version -split '\n')[1] -split ' ')[3]
Write-Host "Detected Qt Version $qtVersion"


# TODO: Remove THIS, only for testing
$qtVersion = "6.2.2"

# Update these to change which artifacts to download!
$avifBuildNum = 51
$apngBuildNum = 66
$kimageformatsBuildNum = 107

# Qt version availability and runner names are assumed.
if ($IsWindows) {
    # TODO
} elseif ($IsMacOS) {
    $imageName = "macos-latest"
} else {
    # TODO
}

$binaryBaseUrl = "https://github.com/jurplel/kimageformats-binaries/releases/download/cont"

if ($pluginNames.count -eq 0) {
    Write-Host "the pluginNames array is empty."
}

foreach ($pluginName in $pluginNames) {
    $artifactName = "$pluginName-$imageName-$qtVersion.zip"
    $downloadUrl = "$binaryBaseUrl/$artifactName"

    Write-Host "Downloading $downloadUrl"
    Invoke-WebRequest -URI $downloadUrl -OutFile $artifactName
    Expand-Archive $artifactName -DestinationPath $pluginName
    Remove-Item $artifactName
}


if ($IsWindows) {
} elseif ($IsMacOS) {
    $out_frm = "bin/qView.app/Contents/Frameworks"
    $out_imf = "bin/qView.app/Contents/PlugIns/imageformats"
} else {
}

mkdir -p "$out_frm"
mkdir -p "$out_imf"

# Copy QtApng
if ($pluginNames -contains 'qtapng') {
    if ($IsMacOS) {
        cp qtapng/QtApng/plugins/imageformats/* "$out/"
    }
}

if ($pluginNames -contains 'kimageformats') {
    if ($IsWindows) {
        # cp kimageformats/kimageformats/output/*.dll "$out/"
    } elseif ($IsMacOS) {
        cp kimageformats/kimageformats/output/*.so "$out_imf/"
        cp kimageformats/kimageformats/output/libKF5Archive.5.dylib "$out_frm/"
    } else {
        # cp kimageformats/kimageformats/output/*.so "$out/"
    }
}
