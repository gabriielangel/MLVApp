#!/bin/sh
#Copy deployed "MLV App.app" to ./App directory before running this
#Copy content from create-dmg (https://github.com/andreyvit/create-dmg) to ./create-dmg-master
test -f "MLVApp.OSX.dmg" && rm "MLVApp.OSX.dmg"
create-dmg \
--volname "MLVApp Installer" \
--volicon "MLVAPP.icns" \
--background "dmg-background.png" \
--window-pos 200 120 \
--window-size 660 400 \
--icon-size 100 \
--icon "MLV App.app" 165 175 \
--hide-extension "MLV App.app" \
--app-drop-link 495 175 \
"MLVApp.OSX.dmg" \
"./app/"