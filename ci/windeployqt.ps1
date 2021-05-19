$ProgressPreference = 'SilentlyContinue'
Invoke-WebRequest https://www.firedaemon.com/download-firedaemon-openssl-x64 -O openssl.zip

7z x -y .\openssl.zip

# Check if "arch" environment variable is win32
# If it is, install x86 binaries, otherwise x64 binaries
If ($env:arch.substring(3, 2) -eq '32') {
    copy openssl-1.1\x86\bin\libssl-1_1.dll bin\
    copy openssl-1.1\x86\bin\libcrypto-1_1.dll bin\
}  Else {
    copy openssl-1.1\x64\bin\libssl-1_1-x64.dll bin\
    copy openssl-1.1\x64\bin\libcrypto-1_1-x64.dll bin\
}

cd bin
windeployqt qView.exe --no-compiler-runtime