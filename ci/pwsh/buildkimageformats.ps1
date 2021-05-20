#! /usr/bin/pwsh

# Clone
git clone https://invent.kde.org/frameworks/kimageformats.git
cd kimageformats
git checkout $(git describe --abbrev=0).substring(0, 7)


# Build

# vcvars on windows
if ($IsWindows) {
    ci/pwsh/vcvars.ps1
}
