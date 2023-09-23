#!/usr/bin/bash

if [ $1 != "" ]; then
        VERSION=$0
else
        VERSION=$(LC_ALL=C sed -n -e '/^VERSION/p' qView.pro)
        VERSION=${VERSION: -3}
fi

cd bin

macdeployqt qView.app
if [ $1 != "" ]; then
    macdeployqt *.app -codesign=- -dmg
    mv qView.dmg qView-JDP-$1.dmg
else
    brew install create-dmg
    codesign --sign - --deep qView.app
    create-dmg --volname "qView $VERSION" --window-size 660 400 --icon-size 160 --icon "qView.app" 180 170 --hide-extension qView.app --app-drop-link 480 170 "qView-JDP-$VERSION.dmg" "qView.app"
fi

rm -r *.app