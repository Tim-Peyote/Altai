"""Joint and loop checks for authored swimming clips, plus reference pose sheets."""
import json,math
from pathlib import Path
import numpy as np
import matplotlib;matplotlib.use('Agg')
import matplotlib.pyplot as plt
from review_recovery_motion import world_pose
from retarget_body_motion import ROOT,rig,R,unit
reports={}
for name in ['SwimBreaststroke','SwimEasy','SwimTread','SwimDrown']:
 d=json.loads((ROOT/'SourceArt/Motion/CMU125'/(name+'.json')).read_text());poses=[world_pose(f) for f in d['frames']];errors=[];joints={}
 for side in ['l','r']:
  for a,b,c in [('thigh','calf','foot'),('upperarm','lowerarm','hand')]:
   a,b,c=[n+'_'+side for n in [a,b,c]];vals=[]
   for p,q in poses:
    u,v=unit(p[b]-p[a]),unit(p[c]-p[b]);vals.append(math.degrees(math.atan2(np.dot(np.cross(u,v),q[a][:,2]),np.dot(u,v))))
   joints[b]=[min(vals),max(vals)]
   if min(vals)<1 or max(vals)>141:errors.append(b)
 maxjump=max(float(np.rad2deg((R.from_quat([f[n]['q'] for f in d['frames'][1:]])*R.from_quat([f[n]['q'] for f in d['frames'][:-1]]).inv()).magnitude()).max()) for n in rig)
 if maxjump>22:errors.append('rotation discontinuity')
 loop_error=0
 if name!='SwimDrown':
  loop_error=max(float(np.rad2deg((R.from_quat(d['frames'][0][n]['q'])*R.from_quat(d['frames'][-1][n]['q']).inv()).magnitude())) for n in rig)
  if loop_error>.01:errors.append('loop seam')
 reports[name]={'passed':not errors,'errors':errors,'joints':joints,'max_frame_rotation':maxjump,'loop_error_degrees':loop_error}
 fig=plt.figure(figsize=(15,4))
 for j,idx in enumerate([0,18,36,54]):
  ax=fig.add_subplot(1,4,j+1,projection='3d');p,q=poses[idx]
  for chain in [['pelvis','spine_03','neck_01','head'],['neck_01','upperarm_l','lowerarm_l','hand_l'],['neck_01','upperarm_r','lowerarm_r','hand_r'],['pelvis','thigh_l','calf_l','foot_l'],['pelvis','thigh_r','calf_r','foot_r']]:
   xyz=np.array([p[n] for n in chain]);ax.plot(xyz[:,0],xyz[:,1],xyz[:,2],marker='o')
  ax.set(xlim=(-100,100),ylim=(-110,110),zlim=(0,180));ax.view_init(20,-40);ax.set_title(str(idx));ax.set_box_aspect((1,1,.8))
 fig.savefig(ROOT/'Saved'/(name+'_poses.png'));plt.close(fig)
result={'passed':all(r['passed'] for r in reports.values()),'clips':reports};(ROOT/'Saved/swim_motion_review.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
