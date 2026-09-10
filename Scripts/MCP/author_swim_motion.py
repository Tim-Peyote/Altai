"""Author constrained breaststroke and economical strokes on Manny.
Technique reference: Swim England pull/breathe/kick/glide; CMU125 timing reference.
This is authored animation, not a claim of underwater motion capture.
"""
import json,math
import numpy as np
from scipy.spatial.transform import Rotation as R
from scipy.interpolate import CubicSpline
from functools import lru_cache
from retarget_body_motion import ROOT,rig,refp,refq,frame,unit,limit_twist
SRC=ROOT/'SourceArt/Motion/CMU125'
@lru_cache(maxsize=128)
def spline(keys):
 times,values=zip(*keys)
 return CubicSpline(times,values,bc_type='periodic' if values[0]==values[-1] else 'clamped')
def curve(t,keys):
 # Shared tangents carry momentum through catch, recovery and kick.
 packed=tuple((a,tuple(x) if isinstance(x,list) else x) for a,x in keys)
 return np.array(spline(packed)(t))
def pose(t,easy,drowning=False,treading=False):
 breath=math.sin(math.pi*min(1,t/.4))**2 if t<.4 else 0
 tilt=R.from_euler('x',(-54-18*t*t*(3-2*t)) if drowning else (-14+2*math.sin(2*math.pi*t) if treading else -78+breath*5+(14 if easy else 0)),degrees=True).as_matrix()
 hip=refp['pelvis'].copy();wp={};wq={}
 for n,(parent,p,q) in rig.items():
  wq[n]=q if parent=='None' else wq[parent]@q
  if n=='pelvis':wq[n]=tilt@refq[n]
  # Lift the chest for the breath, distributed through the thoracic chain.
  if not drowning and not treading and n in ['spine_02','spine_03','spine_04','spine_05']:
   wq[n]=R.from_euler('x',breath*2.0,degrees=True).as_matrix()@wq[n]
  wp[n]=p if parent=='None' else wp[parent]+wq[parent]@p
 def point(p):return hip+tilt@(np.array(p)-refp['pelvis'])
 for side,sign in [('l',1),('r',-1)]:
  if not easy:
   hand=curve(t,[(0,[8,0,196]),(.12,[29,2,187]),(.25,[37,10,165]),(.34,[15,24,150]),(.47,[8,10,179]),(.61,[8,0,196]),(1,[8,0,196])])
   foot=curve(t,[(0,[12,0,10]),(.25,[12,0,10]),(.43,[20,-25,53]),(.52,[26,-20,49]),(.66,[27,-4,21]),(.78,[12,0,10]),(1,[12,0,10])])
  else:
   hand=curve(t,[(0,[14,12,178]),(.25,[32,15,157]),(.5,[14,25,144]),(.78,[14,12,178]),(1,[14,12,178])])
   foot=curve(t,[(0,[12,0,10]),(.24,[12,0,10]),(.46,[20,-22,45]),(.65,[25,-6,23]),(.8,[12,0,10]),(1,[12,0,10])])
  if treading:
   # Small continuous sculling; alternating compact leg support, not forward strokes.
   hand=np.array([30+7*math.sin(2*math.pi*t),20+7*math.cos(2*math.pi*t),133+3*math.sin(2*math.pi*t)])
   leg=(t+(0 if side=='l' else .5))%1
   foot=curve(leg,[(0,[16,-5,36]),(.3,[19,-20,54]),(.6,[24,-5,42]),(1,[16,-5,36])])
  if drowning:
   hand=curve(t,[(0,[14,25,144]),(1,[22,18,108])])
   foot=curve(t,[(0,[20,-22,45]),(1,[15,-7,15])])
  hand[0]*=sign;foot[0]*=sign
  for a,b,e,target,pole in [('upperarm','lowerarm','hand',hand,[sign*52,-12,150]),('thigh','calf','foot',foot,[sign*22,38,55])]:
   a,b,e=[n+'_'+side for n in [a,b,e]];root=wp[a];goal=point(target);pole=point(pole)
   l1=np.linalg.norm(rig[b][1]);l2=np.linalg.norm(rig[e][1]);axis=unit(goal-root);d=np.clip(np.linalg.norm(goal-root),abs(l1-l2)+1,l1+l2-.15)
   mid=(l1*l1-l2*l2+d*d)/(2*d);height=np.sqrt(max(0,l1*l1-mid*mid));bend=unit(pole-root-axis*np.dot(pole-root,axis));joint=root+axis*mid+bend*height
   u=unit(joint-root);v=unit(root+axis*d-joint);normal=unit(np.cross(u,v))
   wq[a]=frame(u,normal)@frame(rig[b][1],np.array([0,0,1])).T;wq[b]=frame(v,normal)@frame(rig[e][1],np.array([0,0,1])).T
   # Preserve positive hinge and bound axial rotation relative to the parent.
   rest=rig[a][2];bounded=wq[rig[a][0]]@rest@limit_twist(rest.T@wq[rig[a][0]].T@wq[a],rig[b][1],95 if a.startswith('upperarm') else 65)
   correction=bounded@wq[a].T;wq[a]=bounded;wq[b]=correction@wq[b];wq[e]=wq[b]@rig[e][2]
 # Point the feet during the glide; prepare the soles for the outward kick.
 for side in ['l','r']:
  kick=float(curve(t,[(0,0),(.22,0),(.43,1),(.59,.7),(.73,0),(1,0)]))
  angle=(-8-12*t) if drowning else -35+45*kick
  n='foot_'+side;wq[n]=R.from_rotvec(tilt@np.array([np.deg2rad(angle),0,0])).as_matrix()@wq[n]
 driven={'pelvis'}|({'spine_02','spine_03','spine_04','spine_05'} if not drowning and not treading else set())|{n+'_'+side for n in ['upperarm','lowerarm','hand','thigh','calf','foot'] for side in ['l','r']}
 result={}
 for n,(parent,p,q) in rig.items():
  if n not in driven:wq[n]=q if parent=='None' else wq[parent]@q
  local=wq[n] if parent=='None' else wq[parent].T@wq[n]
  translation=p.copy()
  if treading and n=='pelvis':translation[2]-=32
  result[n]={'p':translation.tolist(),'q':R.from_matrix(local).as_quat().tolist()}
 return result
for name,easy in [('SwimBreaststroke',False),('SwimEasy',True),('SwimTread',True),('SwimDrown',True)]:
 frames=[pose(i/96,easy,name=='SwimDrown',name=='SwimTread') for i in range(97)]
 (SRC/(name+'.json')).write_text(json.dumps({'fps':30,'authored':True,'technique':'Swim England, CMU125 reference; bounded anatomical IK','frames':frames},separators=(',',':')))
