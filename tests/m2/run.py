import collections,hashlib,json,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2];exe=Path(sys.argv[1]).resolve();results=[]
for c in json.loads((root/'tests/m2/cases.json').read_text()):
 path=root/'tests/m2/fixtures'/(c['name']+'.cpp')
 try:
  check=subprocess.run([str(exe),'check',str(path)],capture_output=True,text=True,timeout=15)
  r=subprocess.run([str(exe),'run',str(path)],capture_output=True,text=True,timeout=15)
  again=subprocess.run([str(exe),'run',str(path)],capture_output=True,text=True,timeout=15)
  expectedCheck=1 if c['phase'] not in [None,'runtime'] else 0
  ok=check.returncode==expectedCheck and not check.stdout and r.returncode==c['exit'] and r.stdout==c['stdout'] and (r.returncode,r.stdout,r.stderr)==(again.returncode,again.stdout,again.stderr)
  ok=ok and ((': '+c['phase']+':') in r.stderr if c['phase'] else not r.stderr)
  detail=dict(check_exit=check.returncode,exit=r.returncode,stdout=r.stdout,stderr=r.stderr)
 except Exception as e:ok=False;detail={'error':str(e)}
 results.append(dict(name=c['name'],category=c['category'],passed=ok,**detail))
 if not ok:print('FAIL',c['name'],detail,flush=True)
report=dict(executable=str(exe),executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),total=len(results),passed=sum(r['passed'] for r in results),categories=dict(collections.Counter(r['category'] for r in results)),results=results)
(root/'audit/m2'/('integration-'+exe.parent.name+'.json')).write_text(json.dumps(report,indent=2));print(f"{report['passed']}/{report['total']} M2 integration cases passed")
raise SystemExit(report['total']!=report['passed'])
