#!/usr/bin/env pwsh

# This script will download binary plugins from the kimageformats-binaries repository using Github's API.

$pluginNames = "qtapng", "kimageformats"

$qtVersion = ((qmake --version -split '\n')[1] -split ' ')[3]
Write-Host "Detected Qt Version $qtVersion"

# Qt version availability and runner names are assumed.
if ($IsWindows) {
    $imageName = "windows-2022"
} elseif ($IsMacOS) {
    $imageName = "macos-12"
} else {
    $imageName = "ubuntu-20.04"
}

$binaryBaseUrl = "https://github.com/jurplel/kimageformats-binaries/releases/download/cont"

if ($pluginNames.count -eq 0) {
    Write-Host "the pluginNames array is empty."
}

foreach ($pluginName in $pluginNames) {
    $qtArch = $env:qtArch ? "-$env:qtArch" : ''
    $artifactName = "$pluginName-$imageName-$qtVersion$qtArch.zip"
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

New-Item -Type Directory -Path "$out_frm" -ErrorAction SilentlyContinue
New-Item -Type Directory -Path "$out_imf" -ErrorAction SilentlyContinue

# Copy QtApng
if ($pluginNames -contains 'qtapng') {
    if ($IsWindows) {
        cp qtapng/QtApng/output/qapng.dll "$out_imf/"
    } elseif ($IsMacOS) {
        cp qtapng/QtApng/output/libqapng.* "$out_imf/"
    } else {
        cp qtapng/QtApng/output/libqapng.so "$out_imf/"
    }
}

if ($pluginNames -contains 'kimageformats') {
    if ($IsWindows) {
        mv kimageformats/kimageformats/output/kimg_*.dll "$out_imf/"
        # Copy karchive
        if (Test-Path -Path kimageformats/kimageformats/output/KF5Archive.dll -PathType Leaf) {
            cp kimageformats/kimageformats/output/zlib1.dll "$out_frm/"
            cp kimageformats/kimageformats/output/KF5Archive.dll "$out_frm/"
        }
        # copy avif stuff
        if (Test-Path -Path kimageformats/kimageformats/output/avif.dll -PathType Leaf) {
            cp kimageformats/kimageformats/output/avif.dll "$out_frm/"
            cp kimageformats/kimageformats/output/aom.dll "$out_frm/"
        }
        # copy heif stuff
        if (Test-Path -Path kimageformats/kimageformats/output/heif.dll -PathType Leaf) {
            cp kimageformats/kimageformats/output/heif.dll "$out_frm/"
            cp kimageformats/kimageformats/output/libde265.dll "$out_frm/"
            if ($env:buildArch -ne 'Arm64') {
                cp kimageformats/kimageformats/output/libx265.dll "$out_frm/"
            }
            cp kimageformats/kimageformats/output/aom.dll "$out_frm/"
        }
        # copy raw stuff
        if (Test-Path -Path kimageformats/kimageformats/output/raw.dll -PathType Leaf) {
            cp kimageformats/kimageformats/output/zlib1.dll "$out_frm/"
            cp kimageformats/kimageformats/output/raw.dll "$out_frm/"
            cp kimageformats/kimageformats/output/lcms2.dll "$out_frm/"
        }
        # copy jxl stuff
        if (Test-Path -Path kimageformats/kimageformats/output/jxl.dll -PathType Leaf) {
            cp kimageformats/kimageformats/output/jxl.dll "$out_frm/"
            cp kimageformats/kimageformats/output/jxl_cms.dll "$out_frm/"
            cp kimageformats/kimageformats/output/jxl_threads.dll "$out_frm/"
            cp kimageformats/kimageformats/output/lcms2.dll "$out_frm/"
            cp kimageformats/kimageformats/output/hwy.dll "$out_frm/"
            cp kimageformats/kimageformats/output/brotlicommon.dll "$out_frm/"
            cp kimageformats/kimageformats/output/brotlidec.dll "$out_frm/"
            cp kimageformats/kimageformats/output/brotlienc.dll "$out_frm/"
        }
        # copy openexr stuff
        if (Test-Path -Path kimageformats/kimageformats/output/OpenEXR-3_2.dll -PathType Leaf) {
            cp kimageformats/kimageformats/output/deflate.dll "$out_frm/"
            cp kimageformats/kimageformats/output/OpenEXR-3_2.dll "$out_frm/"
            cp kimageformats/kimageformats/output/OpenEXRCore-3_2.dll "$out_frm/"
            cp kimageformats/kimageformats/output/Imath-3_1.dll "$out_frm/"
            cp kimageformats/kimageformats/output/IlmThread-3_2.dll "$out_frm/"
            cp kimageformats/kimageformats/output/Iex-3_2.dll "$out_frm/"
        }
    } elseif ($IsMacOS) {
        cp kimageformats/kimageformats/output/*.so "$out_imf/"
        cp kimageformats/kimageformats/output/libKF5Archive.5.dylib "$out_frm/"
    } else {
        cp kimageformats/kimageformats/output/kimg_*.so "$out_imf/"
        cp kimageformats/kimageformats/output/libKF5Archive.so.5 "$out_frm/"
    }
}
