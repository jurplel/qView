param
(
    $NightlyVersion = ""
)

$qtVersion = [version](qmake -query QT_VERSION)
Write-Host "Detected Qt Version $qtVersion"

if ($env:buildArch -ne 'Arm64') {
    # Download and extract openssl
    if ($qtVersion -lt [version]'6.5') {
        $openSslDownloadUrl = "https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-1.1.1w.zip"
        $openSslSubfolder = "openssl-1.1\"
        $openSslFilenameVersion = "1_1"
    } else {
        $openSslDownloadUrl = "https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-3.4.1.zip"
        $openSslSubfolder = ""
        $openSslFilenameVersion = "3"
    }
    Write-Host "Downloading $openSslDownloadUrl"
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $openSslDownloadUrl -OutFile openssl.zip
    7z x -y openssl.zip -o"openssl"

    # Install appropriate binaries for architecture
    if ($env:buildArch -eq 'X86') {
        copy "openssl\${openSslSubfolder}x86\bin\libssl-$openSslFilenameVersion.dll" bin
        copy "openssl\${openSslSubfolder}x86\bin\libcrypto-$openSslFilenameVersion.dll" bin
    } else {
        copy "openssl\${openSslSubfolder}x64\bin\libssl-$openSslFilenameVersion-x64.dll" bin
        copy "openssl\${openSslSubfolder}x64\bin\libcrypto-$openSslFilenameVersion-x64.dll" bin
    }
}

# Run windeployqt
$isCrossCompile = $env:buildArch -eq 'Arm64'
$winDeployQt = $isCrossCompile ? "$(qmake -query QT_HOST_PREFIX)\bin\windeployqt" : "windeployqt"
$argQtPaths = $isCrossCompile ? "--qtpaths=$env:QT_ROOT_DIR\bin\qtpaths.bat" : $null
$argForceOpenSsl = $qtVersion -ge [version]'6.8' ? "--force-openssl" : $null
& $winDeployQt $argQtPaths $argForceOpenSsl --no-compiler-runtime "bin\qView.exe"

$imfDir = "bin\imageformats"
if ((Test-Path "$imfDir\kimg_tga.dll") -and (Test-Path "$imfDir\qtga.dll")) {
    # Prefer kimageformats TGA plugin which supports more formats
    Write-Output "Removing duplicate TGA plugin"
    Remove-Item "$imfDir\qtga.dll"
}

if ($NightlyVersion -eq '') {
    # Call innomake if we are not building a nightly version (no version passed)
    & "dist/scripts/innomake.ps1"
} else {
    # Do renaming-y stuff otherwise
    mv bin\qView.exe "bin\qView-nightly-$NightlyVersion.exe"
}
