param
(
    $NightlyVersion = ""
)

$qtVersion = [version]((qmake --version -split '\n')[1] -split ' ')[3]
Write-Host "Detected Qt Version $qtVersion"

if ($env:buildArch -ne 'Arm64') {
    # Download and extract openssl
    if ($qtVersion.Major -le 5) {
        $openSslDownloadUrl = "https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-1.1.1w.zip"
        $openSslSubfolder = "openssl-1.1\"
        $openSslFilenameVersion = "1_1"
    } else {
        $openSslDownloadUrl = "https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-3.3.2.zip"
        $openSslSubfolder = ""
        $openSslFilenameVersion = "3"
    }
    Write-Host "Downloading $openSslDownloadUrl"
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $openSslDownloadUrl -OutFile openssl.zip
    7z x -y openssl.zip -o"openssl"

    # Install approprate binaries for architecture
    if ($env:buildArch -eq 'X86') {
        copy openssl\$openSslSubfolder\x86\bin\libssl-$openSslFilenameVersion.dll bin\
        copy openssl\$openSslSubfolder\x86\bin\libcrypto-$openSslFilenameVersion.dll bin\
    } else {
        copy openssl\$openSslSubfolder\x64\bin\libssl-$openSslFilenameVersion-x64.dll bin\
        copy openssl\$openSslSubfolder\x64\bin\libcrypto-$openSslFilenameVersion-x64.dll bin\
    }
}

if ($env:buildArch -eq 'Arm64') {
    # Run windeployqt in cross-compilation mode
    $qtHostPath = [System.IO.Path]::GetFullPath("$env:QT_ROOT_DIR\..\$((Split-Path -Path $env:QT_ROOT_DIR -Leaf) -replace '_arm64', '_64')")
    & "$qtHostPath\bin\windeployqt" "--qmake=$env:QT_ROOT_DIR\bin\qmake.bat" --no-compiler-runtime bin\qView.exe
} else {
    # Run windeployqt which should be in path
    windeployqt --no-compiler-runtime bin/qView.exe
}

if ($NightlyVersion -eq '') {
    # Call innomake if we are not building a nightly version (no version passed)
    & "dist/scripts/innomake.ps1"
} else {
    # Do renaming-y stuff otherwise
    mv bin\qView.exe "bin\qView-nightly-$NightlyVersion.exe"
}
