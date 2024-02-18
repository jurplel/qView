param
(
    $NightlyVersion = ""
)

$qtVersion = ((qmake --version -split '\n')[1] -split ' ')[3]
Write-Host "Detected Qt Version $qtVersion"

# Download and extract openssl
if ($qtVersion -like '5.*') {
    $openSslDownloadUrl = "https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-1.1.1w.zip"
    $openSslFolderVersion = "1.1"
    $openSslFilenameVersion = "1_1"
} else {
    $openSslDownloadUrl = "https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-3.2.1.zip"
    $openSslFolderVersion = "3"
    $openSslFilenameVersion = "3"
}
Write-Host "Downloading $openSslDownloadUrl"
$ProgressPreference = 'SilentlyContinue'
Invoke-WebRequest -Uri $openSslDownloadUrl -OutFile openssl.zip
7z x -y .\openssl.zip

# Check if "arch" environment variable is win32
# If it is, install x86 binaries, otherwise x64 binaries
if ($env:arch.substring(3, 2) -eq '32') {
    copy openssl-$openSslFolderVersion\x86\bin\libssl-$openSslFilenameVersion.dll bin\
    copy openssl-$openSslFolderVersion\x86\bin\libcrypto-$openSslFilenameVersion.dll bin\
} else {
    copy openssl-$openSslFolderVersion\x64\bin\libssl-$openSslFilenameVersion-x64.dll bin\
    copy openssl-$openSslFolderVersion\x64\bin\libcrypto-$openSslFilenameVersion-x64.dll bin\
}

# Run windeployqt which should be in path
windeployqt bin/qView.exe --no-compiler-runtime


if ($NightlyVersion -eq '') {
    # Call innomake if we are not building a nightly version (no version passed)
    & "dist/scripts/innomake.ps1"
}
