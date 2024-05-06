#!/usr/bin/bash

if [ $1 != "" ]; then
        VERSION=$0
else
        VERSION=$(LC_ALL=C sed -n -e '/^VERSION/p' qView.pro)
        VERSION=${VERSION: -3}
fi

cd bin

macdeployqt qView.app
codesign --sign - --deep qView.app
if [ $1 != "" ]; then
    BUILD_NAME=qView-nightly-$1
    mv qView.app "$BUILD_NAME.app"
    hdiutil create -volname "$BUILD_NAME" -srcfolder "$BUILD_NAME.app" -fs HFS+ "$BUILD_NAME.dmg"
else
    brew install create-dmg
    create-dmg --volname "qView $VERSION" --window-size 660 400 --icon-size 160 --icon "qView.app" 180 170 --hide-extension qView.app --app-drop-link 480 170 "qView-$VERSION.dmg" "qView.app"
fi

rm -r *.app