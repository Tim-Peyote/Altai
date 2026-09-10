"""Validate actual rendered limb hinges and finite positions through water transitions."""
import json,math,sys
from retarget_body_motion import ROOT,R,rig,unit
import numpy as np
report=sys.argv[1] if len(sys.argv)>1 else 'swimming';p=ROOT/'Saved'/(report+'_review.json');d=json.loads(p.read_text());joints={};fail=[]
for side in ['l','r']:
 for a,b,c in [('thigh','calf','foot'),('upperarm','lowerarm','hand')]:
  a,b,c=[n+'_'+side for n in [a,b,c]];vals=[]
  for r in d['samples']:
   u=unit(np.array(r['bones'][b])-r['bones'][a]);v=unit(np.array(r['bones'][c])-r['bones'][b]);axis=R.from_quat(r['rotations'][a]).apply([0,0,1]);vals.append(math.degrees(math.atan2(np.dot(np.cross(u,v),axis),np.dot(u,v))))
  joints[b]=[min(vals),max(vals)]
  if min(vals)<-1 or max(vals)>145:fail.append(b+' hinge')
for r in d['samples']:
 if not np.isfinite(np.array(list(r['bones'].values()))).all():fail.append('nonfinite pose');break
result={'passed':d['passed'] and not fail,'failures':fail,'joints':joints,'samples':len(d['samples']),'cases':d['cases']}
(ROOT/'Saved'/(report+'_analysis.json')).write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
