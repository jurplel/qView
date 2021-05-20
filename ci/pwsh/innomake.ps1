# Script assumes $env:arch will start with win64 or win32
mkdir ..\dist\win\qView-$env:arch.substring(0, 5)
copy -R bin\* ..\dist\win\qView-$env:arch.substring(0, 5)
iscc ..\dist\win\qView$($env:arch.substring(3, 2)).iss
copy ..\dist\win\Output\* .\