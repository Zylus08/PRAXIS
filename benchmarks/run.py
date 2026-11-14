import json,statistics,subprocess,sys,time
from pathlib import Path
root=Path(__file__).resolve().parents[1]
values=[]
for _ in range(10):
 start=time.perf_counter();r=subprocess.run([sys.argv[1],'run',str(root/'examples/factorial.cpp')],text=True,capture_output=True,check=True);values.append(time.perf_counter()-start)
 assert r.stdout=='3628800'
report={'task':'factorial(10), includes process startup','seconds':values,'median_seconds':statistics.median(values)}
(root/'build/benchmark.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
