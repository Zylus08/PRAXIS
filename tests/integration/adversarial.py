import json,os,subprocess,sys,tempfile,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[2];exe=Path(sys.argv[1]).resolve();results=[]
cases={
 'deep_parentheses':'int main(){return '+'('*600+'1'+')'*600+';}',
 'deep_blocks':'int main(){'+'{'*600+';'+'}'*600+'}',
 'deep_unary':'int main(){return '+'!'*600+'1;}',
 'deep_calls':'int f(int x){return x;}int main(){return '+'f('*600+'1'+')'*600+';}',
 'large_integer':'int main(){return '+'9'*500+';}',
 'large_float':'int main(){return 1e9999;}',
 'missing_comment_end':'int main(){/*',
 'incomplete_escape':'int main(){"abc'+chr(92),
 'embedded_nul':'int main(){'+chr(0)+'}',
 'recursive_macro':'#define A B\n#define B A\nint main(){return A;}',
 'bad_for_initializer':'int main(){for(if(true);;){} }',
 'bad_reference_return':'int& f(int& x){return x;}int main(){}',
}
with tempfile.TemporaryDirectory(dir=root/'build') as d:
 for name,source in cases.items():
  path=Path(d)/(name+'.cpp');path.write_text(source)
  r=subprocess.run([str(exe),'check',str(path)],capture_output=True,text=True,timeout=15)
  results.append(dict(name=name,passed=r.returncode==1 and ': ' in r.stderr,exit=r.returncode,diagnostic=r.stderr))
 # Compiler executables and developer tools are absent from the child's PATH.
 env=dict(os.environ);env['PATH']=os.environ.get('SystemRoot',r'C:\Windows')+'\\System32'
 r=subprocess.run([str(exe),'run',str(root/'examples/factorial.cpp')],env=env,capture_output=True,text=True,timeout=15)
 results.append(dict(name='no_compiler_path',passed=r.returncode==0 and r.stdout=='3628800',exit=r.returncode,stdout=r.stdout,diagnostic=r.stderr))
report=dict(executable=str(exe),executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),passed=sum(r['passed'] for r in results),total=len(results),results=results)
(root/'audit/m2'/('regression-adversarial-'+exe.parent.name+'.json')).write_text(json.dumps(report,indent=2))
print(f"{report['passed']}/{report['total']} adversarial/environment checks passed")
for r in results:
 if not r['passed']:print(r)
raise SystemExit(report['passed']!=report['total'])
