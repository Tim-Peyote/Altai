"""Measure actual rendered limb geometry, independently of solver target success."""
import json,math,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
def sub(a,b):return [x-y for x,y in zip(a,b)]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def norm(v):
 d=math.sqrt(dot(v,v));return [x/d for x in v] if d else [0,0,0]
def rotate(q,v):
 t=[2*x for x in cross(q[:3],v)];c=cross(q[:3],t);return [v[i]+q[3]*t[i]+c[i] for i in range(3)]
def quat_mul(a,b):
 x,y,z,w=a;X,Y,Z,W=b
 return [w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z]
def local_q(parent,child):return quat_mul([-parent[0],-parent[1],-parent[2],parent[3]],child)
def analyze(samples):
 out={}
 for chain in [('upperarm','lowerarm','hand'),('thigh','calf','foot')]:
  for side in ['l','r']:
   a,b,c=[n+'_'+side for n in chain];rows=[];previous=None;max_step=0
   for s in samples:
    if not s.get('mantling',False) and not s.get('attached',False):continue
    q=local_q(s['rotations'][a],s['rotations'][b])
    if previous and previous[1]==s['phase']:
     max_step=max(max_step,math.degrees(2*math.acos(min(1,abs(dot(q,previous[0]))))))
    previous=(q,s['phase'])
    pos=s['bones'];upper=norm(sub(pos[b],pos[a]));lower=norm(sub(pos[c],pos[b]));axis=rotate(s['rotations'][a],[0,0,1])
    angle=math.degrees(math.atan2(dot(cross(upper,lower),axis),dot(upper,lower)))
    off=math.degrees(math.asin(min(1,abs(dot(lower,axis)))))
    rows.append({'flexion':angle,'off_plane':off,'phase':s['phase'],'progress':s['progress']})
   out[b]={'max_frame_rotation':max_step,'min_flexion':min((r['flexion'] for r in rows),default=-999),'max_flexion':max((r['flexion'] for r in rows),default=999),'max_off_plane':max((r['off_plane'] for r in rows),default=999),'worst':max(rows,key=lambda r:r['off_plane'],default={})}
 checks={b:v['min_flexion']>=-.2 and v['max_flexion']<=145.5 and v['max_off_plane']<1 and v['max_frame_rotation']<45 for b,v in out.items()}
 return {'passed':all(checks.values()),'checks':checks,'joints':out}
if __name__=='__main__':
 path=root/'Saved'/(sys.argv[1] if len(sys.argv)>1 else 'mantle_focus_review.json')
 result=analyze(json.loads(path.read_text())['samples']);(root/'Saved/climbing_joint_limits_review.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
