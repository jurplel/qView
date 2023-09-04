# Script assumes $env:arch will start with win64 or win32
$win64orwin32 = $env:arch.substring(0, 5)
New-Item -Path "dist\win\qView-$win64orwin32" -ItemType Directory -ea 0
copy -R bin\* "dist\win\qView-$win64orwin32"
iscc dist\win\qView$($env:arch.substring(3, 2)).iss
copy dist\win\Output\* bin\