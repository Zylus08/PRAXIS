"""Deterministic executable checks; never invokes a compiler."""
import collections,json,subprocess,sys,tempfile,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
exe=Path(sys.argv[1]).resolve()
results=[]
def run(args,input=None):
 return subprocess.run([str(exe),*map(str,args)],input=input,text=True,capture_output=True,timeout=20)
def record(name,category,ok,detail=''):
 results.append(dict(name=name,category=category,passed=bool(ok),detail=detail if not ok else ''))
for c in json.loads((ROOT/'tests/fixtures/cases.json').read_text()):
 p=ROOT/'tests/fixtures'/(c['name']+'.cpp')
 try:
  check=run(['check',p]);r=run(['run',p]);again=run(['run',p])
  static=c['phase'] not in (None,'runtime')
  ok=check.returncode==(1 if static else 0) and check.stdout=='' and r.returncode==c['exit'] and (r.stdout==c['stdout'] if not c['phase'] else True) and (r.returncode,r.stdout,r.stderr)==(again.returncode,again.stdout,again.stderr)
  if c['phase']:ok=ok and ': '+c['phase']+':' in r.stderr
  else:ok=ok and not r.stderr
  record(c['name'],c['category'],ok,repr(dict(check=(check.returncode,check.stdout,check.stderr),run=(r.returncode,r.stdout,r.stderr))))
 except Exception as e:record(c['name'],c['category'],False,str(e))
for args,code,needle in [(['--help'],0,'Usage:'),(['--version'],0,'0.1.0'),(['nonsense'],2,''),(['run'],2,''),(['run','missing.cpp'],1,''),(['run',ROOT/'examples/hello.cpp','--bad'],2,'')]:
 r=run(args);record('cli_'+str(args[0])+str(code)+str(args[-1]),'cli',r.returncode==code and needle in r.stdout,r.stderr)
repls=[
 ('reset','#include <iostream>\nint x=1;\n:reset\n#include <iostream>\nint x=2;\nstd::cout<<x;\n:quit\n','State reset.\n2',''),
 ('runtime_recovery','#include <iostream>\nint x=1;\n{ x=2; int z=0; x/=z; }\nstd::cout<<x;\n:quit\n','2','runtime:'),
 ('array_persistence','#include <iostream>\nint a[2]={1,2};\na[1]=8;\nstd::cout<<a[1];\n:quit\n','8',''),
 ('declarations','#include <iostream>\nint x=2;\nx+=3;\nstd::cout<<x;\n:quit\n','5',''),
 ('functions','#include <iostream>\nint twice(int x){return 2*x;}\nint y=3;\nstd::cout<<twice(y);\n:quit\n','6',''),
 ('references','#include <iostream>\nint x=1;\nint& r=x;\nr=9;\nstd::cout<<x;\n:quit\n','9',''),
 ('invalid_rollback','#include <iostream>\nint x=1;\nint bad=unknown;\nstd::cout<<x;\n:quit\n','1','semantic:'),
 ('macro_persistence','#include <iostream>\n#define N 42\nstd::cout<<N;\n:quit\n','42',''),
 ('multiline','#include <iostream>\nint f(int x){\nreturn x+1;\n}\nstd::cout<<f(2);\n:quit\n','3','')]
for name,input,expected,err in repls:
 r=run(['repl'],input);record('repl_'+name,'repl',r.returncode==0 and r.stdout.split('\n',1)[1]==expected and (err in r.stderr if err else r.stderr==''),repr((r.stdout,r.stderr)))
with tempfile.TemporaryDirectory(dir=ROOT/'build') as tmp:
 d=Path(tmp);(d/'value.hpp').write_text('#define VALUE 37\n');(d/'main.cpp').write_text('#include <iostream>\n#include "value.hpp"\nint main(){std::cout<<VALUE;}')
 r=run(['run',d/'main.cpp']);record('quoted_include','preprocess',r.stdout=='37' and r.returncode==0,r.stderr)
 (d/'main.cpp').write_text('#include <iostream>\n#include <value.hpp>\nint main(){std::cout<<VALUE;}')
 r=run(['run',d/'main.cpp','-I',d]);record('include_path','preprocess',r.stdout=='37' and r.returncode==0,r.stderr)
 (d/'cycle.hpp').write_text('#include "cycle.hpp"\n');(d/'main.cpp').write_text('#include "cycle.hpp"\nint main(){}')
 r=run(['check',d/'main.cpp']);record('include_cycle','preprocess',r.returncode==1 and 'include cycle' in r.stderr,r.stderr)
 (d/'main.cpp').write_text('#include "bad.hpp"\nint main(){}');(d/'bad.hpp').write_text('\nint x=;')
 r=run(['check',d/'main.cpp']);record('include_diagnostic_location','preprocess',r.returncode==1 and 'bad.hpp:2:' in r.stderr,r.stderr)
report=dict(executable=str(exe),executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),total=len(results),passed=sum(r['passed'] for r in results),categories=dict(collections.Counter(r['category'] for r in results)),results=results)
output=ROOT/'audit/m2'/('regression-integration-'+exe.parent.name+'.json');output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(report,indent=2))
for r in results:
 if not r['passed']:print('FAIL',r['name'],r['detail'])
print(f"{report['passed']}/{report['total']} integration checks passed")
sys.exit(0 if report['passed']==report['total'] else 1)
