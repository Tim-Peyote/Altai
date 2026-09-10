"""Retarget public CMU 140 recordings to Manny using rigid segments and signed hinge frames.
Requires numpy/scipy. Input ASF/AMC is motion data, never executed.
"""
from pathlib import Path
import re,json
import numpy as np
from scipy.spatial.transform import Rotation as R
ROOT=Path(__file__).resolve().parents[2]
SRC=ROOT/'SourceArt/Motion/CMU140'
def unit(v):return v/max(np.linalg.norm(v),1e-9)
def frame(x,z):
 x=unit(x);z=unit(z-x*np.dot(z,x));return np.column_stack([x,np.cross(z,x),z])
def blend_rotation(a,b,t):
 return R.from_rotvec((R.from_matrix(b)*R.from_matrix(a).inv()).as_rotvec()*t).as_matrix()@a

def limit_twist(q,axis,degrees):
 """Constrain axial rotation while retaining the captured swing direction."""
 axis=unit(axis);v=R.from_matrix(q).as_quat();twist=np.r_[axis*np.dot(v[:3],axis),v[3]]
 if np.linalg.norm(twist)<1e-7:return q
 twist/=np.linalg.norm(twist);angle=2*np.arctan2(np.dot(twist[:3],axis),twist[3]);angle=(angle+np.pi)%(2*np.pi)-np.pi
 return q@R.from_quat(twist).as_matrix().T@R.from_rotvec(axis*np.clip(angle,-np.deg2rad(degrees),np.deg2rad(degrees))).as_matrix()

def read_asf():
 text=(SRC/'140.asf').read_text();bones={}
 for part in text.split(':bonedata')[1].split(':hierarchy')[0].split('begin')[1:]:
  d={l.split()[0]:l.split()[1:] for l in part.split('end')[0].splitlines() if l.strip()}
  n=d['name'][0];bones[n]={'offset':np.array(d['direction'],float)*float(d['length'][0]),'axis':R.from_euler('xyz',np.array(d['axis'][:3],float),degrees=True).as_matrix(),'dof':d.get('dof',[])}
 parents={}
 for l in text.split(':hierarchy')[1].splitlines():
  v=l.split()
  if len(v)>1:
   for n in v[1:]:parents[n]=v[0]
 return bones,parents
B,P=read_asf()
def read_amc(path):
 frames=[]
 for l in path.read_text().splitlines():
  v=l.split()
  if not v or v[0][0] in ':#':continue
  if v[0].isdigit():frames.append({})
  else:frames[-1][v[0]]=np.array(v[1:],float)
 return frames

def fk(values):
 root=values.get('root',np.zeros(6));pos={'root':root[:3]};rot={'root':R.from_euler('xyz',root[3:],degrees=True).as_matrix()}
 for n,b in B.items():
  angles=np.zeros(3)
  for channel,value in zip(b['dof'],values.get(n,np.zeros(len(b['dof'])))):angles['xyz'.index(channel[-1])]=value
  rot[n]=rot[P[n]]@b['axis']@R.from_euler('xyz',angles,degrees=True).as_matrix()@b['axis'].T
  pos[n]=pos[P[n]]+rot[n]@b['offset']
 return pos,rot
raw=json.loads((ROOT/'Saved/body_rig.json').read_text());rig={};refp={};refq={}
for n,d in raw.items():
 a=list(map(float,re.findall(r'[xyzw]: (-?[0-9.]+)',d['local'])));q=R.from_quat(a[:4]).as_matrix();p=np.array(a[4:7]);parent=d['parent'];rig[n]=(parent,p,q)
 refq[n]=q if parent=='None' else refq[parent]@q;refp[n]=p if parent=='None' else refp[parent]+refq[parent]@p
# CMU is Y-up. Fit the body coordinate frame, not individual Euler angle names.
sr,sg=fk({});source_right=unit(sr['rhipjoint']-sr['lhipjoint']);source_up=np.array([0,1,0]);source_forward=-unit(np.cross(source_right,source_up))
target_right=unit(refp['thigh_r']-refp['thigh_l']);target_up=np.array([0,0,1]);target_forward=unit(np.cross(target_right,target_up))
C=np.column_stack([target_right,target_up,target_forward])@np.column_stack([source_right,source_up,source_forward]).T
# Acclaim and Manny have opposite coordinate handedness. Conjugate rotations
# through this reflection; an angular axis is a pseudovector, so it also needs det(C).
scale=(np.linalg.norm(refp['calf_l']-refp['thigh_l'])+np.linalg.norm(refp['foot_l']-refp['calf_l']))/(np.linalg.norm(B['lfemur']['offset'])+np.linalg.norm(B['ltibia']['offset']))

def retarget(values):
 sp,sq=fk(values);world={};wpos={};locals={}
 groups={'pelvis':'root','spine_01':'lowerback','spine_02':'lowerback','spine_03':'upperback','spine_04':'upperback','spine_05':'thorax','neck_01':'lowerneck','neck_02':'upperneck','head':'head','clavicle_l':'lclavicle','clavicle_r':'rclavicle'}
 # The source rest frames are global identity: Acclaim axis offsets cancel in rest.
 for n,(parent,p,q) in rig.items():
  if n in groups:
   motion=sq[groups[n]]
   if n=='spine_01':motion=blend_rotation(sq['root'],sq['lowerback'],.5)
   if n=='spine_03':motion=blend_rotation(sq['lowerback'],sq['upperback'],.5)
   world[n]=C@motion@C.T@refq[n]
  else:world[n]=q if parent=='None' else world[parent]@q
  wpos[n]=p if parent=='None' else wpos[parent]+world[parent]@p
  if n=='pelvis':wpos[n]=C@sp['root']*scale
 # Define a shared flexion plane for both segments, so the elbow/knee cannot twist independently.
 for side in ['l','r']:
  for upper,joint,end,srcupper,srcjoint in [('thigh','calf','foot','femur','tibia'),('upperarm','lowerarm','hand','humerus','radius')]:
   a,b,e=[k+'_'+side for k in [upper,joint,end]];su=side+srcupper;sj=side+srcjoint
   va=C@(sp[su]-sp[P[su]]);vb=C@(sp[sj]-sp[su]);axis=unit(np.linalg.det(C)*C@sq[su]@B[sj]['axis'][:,0])
   # Signed hinge flexion is anatomically positive in Manny local Z on both sides.
   theta=np.clip(np.arccos(np.clip(np.dot(unit(va),unit(vb)),-1,1)),np.deg2rad(2),np.deg2rad(140))
   va=unit(va);vb=R.from_rotvec(axis*theta).apply(va)
   for name,v,child in [(a,va,b),(b,vb,e)]:
    localdir=rig[child][1];world[name]=frame(v,axis)@frame(localdir,np.array([0,0,1])).T
   world[e]=world[b]@rig[e][2]
   # Retain captured reach, but never pay for it with an inverted shoulder/hip.
   parent=rig[a][0];rest=rig[a][2];local=world[parent].T@world[a]
   bounded=world[parent]@rest@limit_twist(rest.T@local,rig[b][1],100 if upper=='upperarm' else 75)
   correction=bounded@world[a].T;world[a]=bounded;world[b]=correction@world[b];world[e]=correction@world[e]
 # Final hierarchy restores all non-animated descendants, especially twist bones/fingers.
 driven=set(groups)|{n+'_'+side for n in ['thigh','calf','foot','upperarm','lowerarm','hand'] for side in ['l','r']}
 for n,(parent,p,q) in rig.items():
  if n not in driven:world[n]=q if parent=='None' else world[parent]@q
  localq=world[n] if parent=='None' else world[parent].T@world[n]
  locals[n]={'q':R.from_matrix(localq).as_quat().tolist(),'p':(wpos['pelvis'] if n=='pelvis' else p).tolist()}
 return locals

def build(number,label):
 frames=read_amc(SRC/f'140_{number:02d}.amc')[::4];poses=[retarget(v) for v in frames]
 # Remove initial idle and final idle using pelvis height/limb speed; retain natural anticipation.
 h=np.array([v['pelvis']['p'][2] for v in poses]);low=np.where(h<np.max(h)*.55)[0];high=np.where(h>np.max(h)*.93)[0]
 first=0
 last=min(len(poses)-1,int(high[high>first][0])+20) if np.any(high>first) else len(poses)-1
 poses=poses[first:last+1]
 # End facing +Y in mesh coordinates (actor forward). Keep displacement within the recording.
 endq=R.from_quat(poses[-1]['pelvis']['q']).as_matrix()@refq['pelvis'].T
 facing=endq@target_forward;yaw=np.arctan2(np.cross(facing,target_forward)[2],np.dot(facing[:2],target_forward[:2]))
 align=R.from_rotvec(np.array([0,0,yaw])).as_matrix();endpos=align@np.array(poses[-1]['pelvis']['p']);offset=endpos*np.array([1,1,0])
 # Retarget height: sole reference floor and proportions of the destination skeleton.
 zshift=refp['pelvis'][2]-poses[-1]['pelvis']['p'][2]
 for pose in poses:
  pose['pelvis']['p']=(align@np.array(pose['pelvis']['p'])-offset+np.array([0,0,zshift])).tolist()
  pose['pelvis']['q']=R.from_matrix(align@R.from_quat(pose['pelvis']['q']).as_matrix()).as_quat().tolist()
 # Finish the captured crouched stance by extending hips/knees over planted feet.
 # This authored tail is explicit; it is not advertised as part of the CMU recording.
 start=poses[-1];relaxed={}
 for side in ['l','r']:
  a,b,e=['upperarm_'+side,'lowerarm_'+side,'hand_'+side];normal=np.array([1,0,0])
  upper=unit(np.array([.06 if side=='l' else -.06,0,-1]));lower=R.from_rotvec(normal*np.deg2rad(15)).apply(upper)
  uq=frame(upper,normal)@frame(rig[b][1],np.array([0,0,1])).T
  lq=frame(lower,normal)@frame(rig[e][1],np.array([0,0,1])).T
  relaxed[a]=refq[rig[a][0]].T@uq;relaxed[b]=uq.T@lq;relaxed[e]=rig[e][2]
 for j in range(1,31):
  t=j/30;alpha=t*t*(3-2*t);f={}
  for n,(parent,p,q) in rig.items():
   a=R.from_quat(start[n]['q']);target=R.from_matrix(q)
   if n in relaxed:target=R.from_matrix(relaxed[n])
   elif n.startswith(('finger','thumb','index','middle','ring','pinky')):target=a
   delta=(target*a.inv()).as_rotvec();f[n]={'q':(R.from_rotvec(delta*alpha)*a).as_quat().tolist(),'p':start[n]['p'][:]}
  f['pelvis']['p']=(np.array(start['pelvis']['p'])*(1-alpha)+refp['pelvis']*alpha).tolist();poses.append(f)
 # Remove capture jitter without a temporal lag. Contact correction follows filtering.
 for n in rig:
  qs=np.array([f[n]['q'] for f in poses]);ps=np.array([f[n]['p'] for f in poses])
  for i in range(1,len(qs)):
   if np.dot(qs[i-1],qs[i])<0:qs[i]*=-1
  padded=np.pad(qs,((2,2),(0,0)),mode='edge');paddedp=np.pad(ps,((2,2),(0,0)),mode='edge')
  for i,f in enumerate(poses):
   q=np.array([1,2,3,2,1])@padded[i:i+5]/9;f[n]['q']=(q/np.linalg.norm(q)).tolist()
   if n=='pelvis':f[n]['p']=(np.array([1,2,3,2,1])@paddedp[i:i+5]/9).tolist()
 # Keep the support geometry above the ground while retaining real recorded timing.
 for f in poses:
  wp={};wq={}
  for n,(parent,_,__) in rig.items():
   q=R.from_quat(f[n]['q']).as_matrix();p=np.array(f[n]['p']);wq[n]=q if parent=='None' else wq[parent]@q;wp[n]=p if parent=='None' else wp[parent]+wq[parent]@p
  contact=min(wp[n][2]-(8 if n.startswith('foot') else 4) for n in ['foot_l','foot_r','hand_l','hand_r','pelvis','head'])
  f['pelvis']['p'][2]-=contact
 # Retargeting different proportions can leave both feet above the floor.
 # Keep the lower foot as support; preserve the other foot's recorded reach/step.
 for f in poses:
  wp={};wq={}
  for n,(parent,_,__) in rig.items():
   q=R.from_quat(f[n]['q']).as_matrix();p=np.array(f[n]['p']);wq[n]=q if parent=='None' else wq[parent]@q;wp[n]=p if parent=='None' else wp[parent]+wq[parent]@p
  low=min(wp['foot_l'][2],wp['foot_r'][2])
  for side in ['l','r']:
   a,b,e=['thigh_'+side,'calf_'+side,'foot_'+side];A,K,E=[wp[n] for n in [a,b,e]]
   fade=np.clip((E[2]-low-10)/15,0,1);weight=1-fade*fade*(3-2*fade)
   target=E.copy();target[2]=E[2]*(1-weight)+8*weight
   length1=np.linalg.norm(rig[b][1]);length2=np.linalg.norm(rig[e][1]);axis=unit(target-A)
   distance=np.linalg.norm(target-A)
   min_d=np.sqrt(length1**2+length2**2+2*length1*length2*np.cos(np.deg2rad(140)))
   max_d=np.sqrt(length1**2+length2**2+2*length1*length2*np.cos(np.deg2rad(2)))
   distance=np.clip(distance,min_d,max_d);target=A+axis*distance
   hinge=wq[a]@np.array([0,0,1]);hinge=unit(hinge-axis*np.dot(hinge,axis))
   bend=unit(np.cross(axis,hinge));along=(length1**2-length2**2+distance**2)/(2*distance)
   knee=A+axis*along+bend*np.sqrt(max(0,length1**2-along**2));normal=unit(np.cross(knee-A,target-knee))
   upperq=frame(knee-A,normal)@frame(rig[b][1],np.array([0,0,1])).T
   lowerq=frame(target-knee,normal)@frame(rig[e][1],np.array([0,0,1])).T
   f[a]['q']=R.from_matrix(wq[rig[a][0]].T@upperq).as_quat().tolist()
   f[b]['q']=R.from_matrix(upperq.T@lowerq).as_quat().tolist()
   ankle=rig[e][2].T@lowerq.T@wq[e];rv=R.from_matrix(ankle).as_rotvec();angle=np.linalg.norm(rv)
   # A foot cannot cancel a leg correction by spinning around the ankle.
   if angle>np.deg2rad(50):rv*=np.deg2rad(50)/angle
   f[e]['q']=R.from_matrix(rig[e][2]@R.from_rotvec(rv).as_matrix()).as_quat().tolist()
  # Flatten a supporting palm instead of balancing on fingertips. Forearm roll
  # cannot be paid for by unlimited wrist twist; keep an anatomical wrist envelope.
  for side in ['l','r']:
   n='hand_'+side;contact=np.clip((18-wp[n][2])/12,0,1);contact=contact*contact*(3-2*contact)
   if contact<=0:continue
   index=rig['index_metacarpal_'+side][1];pinky=rig['pinky_metacarpal_'+side][1]
   forward=unit(index+pinky);normal=unit(np.cross(index-pinky,forward))*(1 if side=='l' else -1)
   tangent=wq[n]@forward;tangent[2]=0
   if np.linalg.norm(tangent)<.1:tangent=target_forward.copy()
   target=frame(tangent,np.array([0,0,-1]))@frame(forward,normal).T
   local=wq[rig[n][0]].T@target;delta=limit_twist(rig[n][2].T@local,forward,15)
   rv=R.from_matrix(delta).as_rotvec();angle=np.linalg.norm(rv)
   if angle>np.deg2rad(70):rv*=np.deg2rad(70)/angle
   bounded=rig[n][2]@R.from_rotvec(rv).as_matrix()
   f[n]['q']=R.from_matrix(blend_rotation(R.from_quat(f[n]['q']).as_matrix(),bounded,contact)).as_quat().tolist()
 # Contact weights can change faster than the capture. Ease wrist acquisition too.
 for n in ['hand_l','hand_r']:
  qs=np.array([f[n]['q'] for f in poses])
  for i in range(1,len(qs)):
   if np.dot(qs[i-1],qs[i])<0:qs[i]*=-1
  qs=np.pad(qs,((2,2),(0,0)),mode='edge')
  for i,f in enumerate(poses):
   q=np.array([1,2,3,2,1])@qs[i:i+5];f[n]['q']=(q/np.linalg.norm(q)).tolist()
 out={'fps':30,'source':f'CMU 140_{number:02d}','label':label,'frames':poses}
 (SRC/(label+'.json')).write_text(json.dumps(out,separators=(',',':')))
 print(label,len(frames),first,last,'seconds',(len(poses)-1)/30,'pelvis',poses[0]['pelvis']['p'],poses[-1]['pelvis']['p'])
 return out
if __name__=='__main__':
 for num,label in [(1,'GetUpProne'),(8,'GetUpSupine')]:build(num,label)
