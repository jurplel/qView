$suffix =
    $env:buildArch -eq 'X86' ? '32' :
    $env:buildArch -eq 'Arm64' ? 'Arm64' :
    '64';
New-Item -Path "dist\win\qView-Win$suffix" -ItemType Directory -ea 0
copy -R bin\* "dist\win\qView-Win$suffix"
iscc dist\win\qView$suffix.iss
copy dist\win\Output\* bin\