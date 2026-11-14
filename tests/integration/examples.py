import hashlib,json,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2];exe=Path(sys.argv[1]).resolve()
expected={'hello':'Hello World\n','arithmetic':'14','conditions':'1','loops':'012345\n','factorial':'3628800','fibonacci':'55','functions':'72','scopes':'21','arrays':'10','references':'9','strings':'hi!'}
rows=[]
for name in [*expected,'error_diagnostics']:
 path=root/'examples'/(name+'.cpp')
 r=subprocess.run([str(exe),'run',str(path)],text=True,capture_output=True,timeout=20)
 c=subprocess.run([str(exe),'check',str(path)],text=True,capture_output=True,timeout=20)
 good=(r.returncode==0 and r.stdout==expected[name] and not r.stderr and c.returncode==0 and not c.stdout) if name in expected else (r.returncode==1 and ': semantic:' in r.stderr and c.returncode==1)
 rows.append(dict(example=name,passed=good,run_exit=r.returncode,stdout=r.stdout,stderr=r.stderr,check_exit=c.returncode))
for arg in ['--help','--version']:
 r=subprocess.run([str(exe),arg],text=True,capture_output=True,timeout=20);rows.append(dict(command=arg,passed=r.returncode==0,stdout=r.stdout,stderr=r.stderr))
report=dict(executable=str(exe),executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),passed=sum(r['passed'] for r in rows),total=len(rows),results=rows)
(root/'audit/m1/cli-demonstrations.json').write_text(json.dumps(report,indent=2))
print(report['passed'],'/',report['total'],'CLI demonstrations passed')
raise SystemExit(report['passed']!=report['total'])
