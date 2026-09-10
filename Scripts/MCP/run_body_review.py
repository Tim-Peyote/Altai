"""Run body integration cases, accepting only freshly written reports."""
import json,time,sys
from furniture_console import run,ROOT
cases=[('review_body_dynamics.py','body_dynamics_review.json',25),('review_body_scenarios.py','body_scenarios_review.json',90),('review_trip_obstacles.py','trip_obstacles_review.json',45),('review_recovery_terrain.py','recovery_terrain_review.json',160)]
if len(sys.argv)>1:cases=[c for c in cases if c[0] in sys.argv[1:]]
results=[]
for script,name,limit in cases:
 path=ROOT/'Saved'/name;old=path.stat().st_mtime_ns if path.exists() else 0
 run(script);print('START',script,flush=True);end=time.monotonic()+limit
 while time.monotonic()<end and (not path.exists() or path.stat().st_mtime_ns==old):time.sleep(.5)
 if not path.exists() or path.stat().st_mtime_ns==old:raise TimeoutError(script)
 data=json.loads(path.read_text());result={'script':script,'passed':data.get('passed',False),'error':data.get('error','')};results.append(result);print(json.dumps(result),flush=True)
 if not result['passed']:break
(ROOT/'Saved/body_suite_summary.json').write_text(json.dumps(results,indent=2))
