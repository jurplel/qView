using namespace System.Runtime.InteropServices

if ([RuntimeInformation]::OSArchitecture -ne [Architecture]::X64) {
    throw 'Unsupported host architecture.'
}

$arch =
    $env:buildArch -eq 'X86' ? 'x64_x86' :
    $env:buildArch -eq 'Arm64' ? 'x64_arm64' :
    'x64'
$path = Resolve-Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio\*\*\VC\Auxiliary\Build" | select -ExpandProperty Path

cmd.exe /c "call `"$path\vcvarsall.bat`" $arch && set > %temp%\vcvars.txt"

Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
  if ($_ -match "^(.*?)=(.*)$") {
    Set-Content "env:\$($matches[1])" $matches[2]
  }
}