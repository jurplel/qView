#! /usr/bin/pwsh

try {
    if ($IsWindows) {
        copy ..\QtApng\plugins\imageformats\qapng.dll bin\imageformats\

        copy ..\libde265\libde265\libde265.dll bin\
        copy ..\libde265\libheif\libheif\heif.dll bin\
        copy ..\libde265\libheif\qt-heif-image-plugin\bin\imageformats\qheif.dll bin\imageformats\
        
        copy ..\qt-avif-image-plugin\plugins\imageformats\qavif.dll bin\imageformats\
    } else if ($IsMacOS) {
        cp $env:Qt5_DIR/plugins/imageformats/libqapng.dylib qView.app/Contents/PlugIns/imageformats/
        cp $env:Qt5_DIR/plugins/imageformats/libqavif.dylib qView.app/Contents/PlugIns/imageformats/
    } else {

    }

    Write-Host "Successfully copied plugins"
} catch {            
    Write-Host "Failed to copy plugins"
    echo $Error[0]
}