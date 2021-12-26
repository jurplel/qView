# Script assumes $env:arch will start with win64 or win32
# This should probably be an arg
$arch = $env:arch.substring(3, 2)
$path = Resolve-Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio\*\*\VC\Auxiliary\Build" | select -ExpandProperty Path

cmd.exe /c "call `"$path\vcvars$arch.bat`" && set > %temp%\vcvars.txt"

Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
  if ($_ -match "^(.*?)=(.*)$") {
    Set-Content "env:\$($matches[1])" $matches[2]
  }
}