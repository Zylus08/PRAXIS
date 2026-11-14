param([ValidateSet('Debug','Release')][string]$Configuration='Debug')
$ErrorActionPreference='Stop'
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'MSVC C++ toolchain not found' }
$dev=Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$cmake=Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ninja=Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$envLines=& cmd /d /c "call `"$dev`" -arch=x64 >nul && set"
foreach($entry in $envLines) { if($entry -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1],$matches[2],'Process') } }
& $cmake -S . -B "build/$Configuration" -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_BUILD_TYPE=$Configuration" "-DCMAKE_CXX_FLAGS_DEBUG=/Od /Z7 /RTC1"
if($LASTEXITCODE) {exit $LASTEXITCODE}
& $cmake --build "build/$Configuration" --parallel 1
exit $LASTEXITCODE
