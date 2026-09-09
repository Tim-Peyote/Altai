"""Run the climbing integration helpers serially in an existing lab PIE session."""
import json,time,sys
from pathlib import Path
from furniture_console import run,ROOT
cases=[('review_climbing_cycle.py','climbing_cycle_review.json',65),('review_mantle_focus.py','mantle_focus_review.json',20),('review_climbing_camera.py','climbing_camera_review.json',25),('review_ground_mantles.py','ground_mantles_review.json',30),('review_climbing_boundaries.py','climbing_boundaries_review.json',90)]
cases.extend([('review_climbing_entry.py','climbing_entry_review.json',15),('review_climbing_aim.py','climbing_aim_review.json',70),('validate_continuous_climb.py','continuous_climb_validation.json',245),('review_object_manipulation.py','object_manipulation_review.json',65)])
if len(sys.argv)>1:cases=[c for c in cases if c[0] in sys.argv[1:]]
results=[]
for script,report,timeout in cases:
 p=ROOT/'Saved'/report;old=p.stat().st_mtime_ns if p.exists() else 0
 run(script);print('START',script,flush=True);end=time.monotonic()+timeout
 while time.monotonic()<end and (not p.exists() or p.stat().st_mtime_ns==old):time.sleep(1)
 if not p.exists() or p.stat().st_mtime_ns==old:raise TimeoutError(script)
 d=json.loads(p.read_text());results.append({'script':script,'passed':d.get('passed',False),'error':d.get('error','')});print('RESULT',json.dumps(results[-1]),flush=True)
 if not results[-1]['passed']:break
(ROOT/'Saved'/'climbing_suite_summary.json').write_text(json.dumps(results,indent=2))
