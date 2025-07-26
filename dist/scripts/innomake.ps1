$arch = $env:buildArch
$suffix = if ($arch -eq 'x86') { '32' } elseif ($arch -eq 'arm64') { 'Arm64' } else { '64' }

New-Item -Path "dist\win\qView-Win$suffix" -ItemType Directory -ea 0
Copy-Item -Path bin\* -Destination "dist\win\qView-Win$suffix" -Recurse

$iss_args = @()
if ($arch -eq 'x86') {
    $iss_args += '/dMyArch=x86'
} elseif ($arch -eq 'arm64') {
    $iss_args += '/dMyArch=arm64'
}

iscc.exe $iss_args "dist\win\qView.iss"
copy dist\win\Output\* bin\