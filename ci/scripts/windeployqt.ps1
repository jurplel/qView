param
(
    $NightlyVersion = ""
)

# Download and extract openssl
$ProgressPreference = 'SilentlyContinue'
Invoke-WebRequest https://www.firedaemon.com/download-firedaemon-openssl-1-zip -O openssl.zip
7z x -y .\openssl.zip

# Check if "arch" environment variable is win32
# If it is, install x86 binaries, otherwise x64 binaries
if ($env:arch.substring(3, 2) -eq '32') {
    copy openssl-1.1\x86\bin\libssl-1_1.dll bin\
    copy openssl-1.1\x86\bin\libcrypto-1_1.dll bin\
}  Else {
    copy openssl-1.1\x64\bin\libssl-1_1-x64.dll bin\
    copy openssl-1.1\x64\bin\libcrypto-1_1-x64.dll bin\
}

# Run windeployqt which should be in path
windeployqt bin/qView.exe --no-compiler-runtime

# Do renaming-y stuff
mv bin\qView.exe "bin\qView-nightly-$NightlyVersion.exe"

# Call innomake if we are not building a nightly version (no version passed)
if ($NightlyVersion -ne '') {
    & "ci/scripts/innomake.ps1"
}