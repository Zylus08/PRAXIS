param([ValidateSet('Debug','Release')][string]$Configuration='Debug',[switch]$Differential,[switch]$ReuseReference)
$ErrorActionPreference='Stop'
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dev=Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$envLines=& cmd /d /c "call `"$dev`" -arch=x64 >nul && set"
foreach($entry in $envLines) { if($entry -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1],$matches[2],'Process') } }
$ctest=Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
& $ctest --test-dir "build/$Configuration" --output-on-failure
if($LASTEXITCODE) {exit $LASTEXITCODE}
if($Differential) {
 if(($Configuration -eq 'Release' -or $ReuseReference) -and (Test-Path 'audit/m1/differential-Debug.json')) {
  python tests/differential/run.py "build/$Configuration/praxis.exe" --reference-report audit/m1/differential-Debug.json
 } else { python tests/differential/run.py "build/$Configuration/praxis.exe" }
 exit $LASTEXITCODE
}
