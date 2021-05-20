#! /usr/bin/pwsh

# Clone QtApng
git clone https://github.com/Skycoder42/QtApng
cd QtApng
git checkout $(git tag | tail -1)

