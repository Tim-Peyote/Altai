"""Independent review of rendered joints and discontinuities in body test samples."""
import json,math
from pathlib import Path
from analyze_climbing_joints import sub,dot,cross,norm,rotate,local_q
root=Path(__file__).resolve().parents[2]
d=json.loads((root/'Saved/body_dynamics_review.json').read_text());rows=d['samples'];out={}
for side in ['l','r']:
 for a,b,c in [('thigh','calf','foot'),('upperarm','lowerarm','hand')]:
  a,b,c=[x+'_'+side for x in [a,b,c]];values=[]
  for s in rows:
   u=norm(sub(s['bones'][b],s['bones'][a]));v=norm(sub(s['bones'][c],s['bones'][b]));axis=rotate(s['rotations'][a],[0,0,1])
   angle=math.degrees(math.atan2(dot(cross(u,v),axis),dot(u,v)))
   values.append((angle,math.degrees(math.asin(min(1,abs(dot(v,axis))))),s['state'],s['t']))
  out[b]={'min':min(values),'max':max(values),'max_off_plane':max(v[1] for v in values)}
steps=[]
for a,b in zip(rows,rows[1:]):
 if 'GETTING_UP' in b['state']:
  steps.append((math.sqrt(dot(sub(a['bones']['pelvis'],b['bones']['pelvis']),sub(a['bones']['pelvis'],b['bones']['pelvis']))),b['t'],b['progress']))
result={'joints':out,'max_recovery_pelvis_step':max(steps,default=None),'runtime':{k:v for k,v in d.items() if k!='samples'}}
(root/'Saved/body_joint_review.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
