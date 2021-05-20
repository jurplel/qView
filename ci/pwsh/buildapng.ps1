#! /usr/bin/pwsh

# Clone
git clone https://github.com/Skycoder42/QtApng
cd QtApng
git checkout $(git tag | select -last 1)



# Build

# vcvars on windows
if ($IsWindows) {
    ci/pwsh/vcvars.ps1
}

qmake "CONFIG += libpng_static"
if ($IsWindows) {
    nmake
} else {
    make
    sudo make install
}
cd ..
