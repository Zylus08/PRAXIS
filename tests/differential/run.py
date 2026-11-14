"""External reference verification only. No compiler is linked into Praxis."""
import argparse,json,subprocess,shutil,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('praxis');p.add_argument('--compiler',default='cl');p.add_argument('--reference-report');args=p.parse_args()
prior=json.loads(Path(args.reference_report).read_text()) if args.reference_report else {}
cached={r['name']:r for r in prior.get('results',[])}
exe=Path(args.praxis).resolve();work=ROOT/'build/differential';work.mkdir(parents=True,exist_ok=True);results=[]
for c in json.loads((ROOT/'tests/fixtures/cases.json').read_text()):
 if c['phase'] is not None:continue
 source=ROOT/'tests/fixtures'/(c['name']+'.cpp');target=work/(c['name']+'.exe')
 command=[args.compiler,'/nologo','/std:c++20','/EHsc','/Od',str(source),'/Fe:'+str(target),'/Fo:'+str(work/(c['name']+'.obj'))]
 digest=hashlib.sha256(source.read_bytes()).hexdigest()
 cache=cached.get(c['name'])
 reuse=bool(cache and cache.get('source_sha256')==digest and cache.get('compile_exit')==0 and 'reference_stdout' in cache)
 compile=subprocess.CompletedProcess(command,0,'','') if reuse else subprocess.run(command,capture_output=True,text=True,timeout=60)
 row=dict(name=c['name'],source_sha256=digest,reference_reused=reuse,compile_exit=compile.returncode)
 if compile.returncode:row.update(passed=False,stage='reference_compile',diagnostic=compile.stdout+compile.stderr)
 else:
  check=subprocess.run([str(exe),'check',str(source)],capture_output=True,text=True,timeout=20)
  native=subprocess.CompletedProcess([str(target)],cache['reference_exit'],cache['reference_stdout'],'') if reuse else subprocess.run([str(target)],capture_output=True,text=True,timeout=20)
  praxis=subprocess.run([str(exe),'run',str(source)],capture_output=True,text=True,timeout=20)
  repeat=subprocess.run([str(exe),'run',str(source)],capture_output=True,text=True,timeout=20)
  row.update(stage='comparison',passed=check.returncode==0 and (praxis.returncode,praxis.stdout)==(native.returncode,native.stdout)==(repeat.returncode,repeat.stdout) and not praxis.stderr,reference_stdout=native.stdout,praxis_stdout=praxis.stdout,reference_exit=native.returncode,praxis_exit=praxis.returncode,check_exit=check.returncode,diagnostic=check.stderr+praxis.stderr)
 results.append(row)
 print(('PASS' if row['passed'] else 'FAIL'),c['name'],flush=True)
report=dict(compiler=prior.get('compiler') or shutil.which(args.compiler) or args.compiler,executable=str(exe),executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),total=len(results),passed=sum(r['passed'] for r in results),results=results)
out=ROOT/'audit/m1'/('differential-'+exe.parent.name+'.json');out.write_text(json.dumps(report,indent=2));print(f"{report['passed']}/{report['total']} differential programs passed")
raise SystemExit(0 if report['passed']==report['total'] else 1)
