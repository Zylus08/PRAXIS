import subprocess, os
from pathlib import Path

tests = [
    'accumulator_template.cpp',
    'acc_double.cpp',
    'acc_long_long.cpp',
    'acc_multiple_instantiations.cpp',
    'acc_multiple_instances_scope.cpp',
    'acc_const_member.cpp',
    'acc_container_member.cpp',
    'sum_container.cpp',
    'acc_copy.cpp',
]

os.makedirs('build/rc1_1_ref', exist_ok=True)
vcvars = r'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul && '

all_passed = True
for t in tests:
    src = f'examples/rc1_1/{t}'
    obj = f'build/rc1_1_ref/{t}.obj'
    exe = f'build/rc1_1_ref/{t}.exe'
    compile_cmd = f'{vcvars} cl /nologo /std:c++20 /EHsc /O2 {src} /Fe:{exe} /Fo:{obj}'
    res = subprocess.run(compile_cmd, shell=True, capture_output=True, text=True)
    if res.returncode != 0:
        print(f'MSVC COMPILE FAILED for {t}: {res.stderr}')
        all_passed = False
        continue
    msvc_run = subprocess.run([exe], capture_output=True)
    praxis_run = subprocess.run(['build/Release/praxis.exe', 'run', src], capture_output=True)
    match = (msvc_run.stdout == praxis_run.stdout) and (msvc_run.returncode == praxis_run.returncode == 0)
    if not match:
        all_passed = False
    print(f'[{ "PASS" if match else "FAIL" }] {t}: praxis={praxis_run.stdout!r} msvc={msvc_run.stdout!r}')

print("ALL ACCEPTANCE TESTS PASSED:", all_passed)
exit(0 if all_passed else 1)
