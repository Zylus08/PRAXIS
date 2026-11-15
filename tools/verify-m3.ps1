param([ValidateSet('Debug','Release')][string]$Configuration='Debug')
$ErrorActionPreference='Stop'
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dev=Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$envLines=& cmd /d /c "call `"$dev`" -arch=x64 >nul && set"
foreach($entry in $envLines){if($entry -match '^([^=]+)=(.*)$'){[Environment]::SetEnvironmentVariable($matches[1],$matches[2],'Process')}}
$ctest=Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
& $ctest --test-dir "build/$Configuration" --output-on-failure
if($LASTEXITCODE){exit $LASTEXITCODE}
python tests/integration/adversarial.py "build/$Configuration/praxis.exe"
if($LASTEXITCODE){exit $LASTEXITCODE}
python tests/differential/run.py "build/$Configuration/praxis.exe" --reference-report audit/m1/differential-Debug.json
if($LASTEXITCODE){exit $LASTEXITCODE}
if(Test-Path 'audit/m2/differential-Debug.json'){
 python tests/m2/differential.py "build/$Configuration/praxis.exe" --reference-report audit/m2/differential-Debug.json
}else{python tests/m2/differential.py "build/$Configuration/praxis.exe"}
if($LASTEXITCODE){exit $LASTEXITCODE}
python tests/m3/run.py "build/$Configuration/praxis.exe"
if($LASTEXITCODE){exit $LASTEXITCODE}
if(Test-Path "audit/m3/differential-Debug.json"){
 python tests/m3/differential.py "build/$Configuration/praxis.exe" --reference-report audit/m3/differential-Debug.json
}else{python tests/m3/differential.py "build/$Configuration/praxis.exe"}
exit $LASTEXITCODE
