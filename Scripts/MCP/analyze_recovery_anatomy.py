"""Measure proximal joints in actual PIE recovery samples, including acquisition."""
import json
import numpy as np
from retarget_body_motion import ROOT, R, rig, unit

def audit(rows):
    rows=[r for r in rows if 'GETTING_UP' in r['state']]
    result={}
    for side in ['l','r']:
        for name,child in [('thigh','calf'),('upperarm','lowerarm')]:
            n=name+'_'+side;parent=rig[n][0];axis=unit(rig[child+'_'+side][1]);values=[]
            for r in rows:
                q=(R.from_quat(r['rotations'][parent]).inv()*R.from_quat(r['rotations'][n]))
                v=(R.from_matrix(rig[n][2]).inv()*q).as_quat()
                angle=np.rad2deg(2*np.arctan2(np.dot(v[:3],axis),v[3]));angle=(angle+180)%360-180
                values.append((float(angle),r['progress']))
            result[n]={'min':min(values,default=None),'max':max(values,default=None)}
    rotations={}
    for n in ['spine_01','spine_02','spine_03','spine_04','spine_05','neck_01','neck_02','head','foot_l','foot_r','hand_l','hand_r']:
        angles=[]
        for r in rows:
            q=R.from_matrix(rig[n][2]).inv()*R.from_quat(r['rotations'][rig[n][0]]).inv()*R.from_quat(r['rotations'][n])
            angles.append(float(np.rad2deg(q.magnitude())))
        rotations[n]=max(angles,default=999)
    bridges=[]
    for i,r in enumerate(rows):
        if r['face_up'] and r['progress']>.2:
            # Relative heights make this independent of the disposable floor's elevation.
            p=r['bones'];hip=np.array(p['pelvis']);feet=min(p['foot_l'][2],p['foot_r'][2])
            if hip[2]-feet>35 and p['head'][2]<hip[2]+10:bridges.append(i)
    return {'joints':result,'back_bridge_frames':bridges,'samples':len(rows),'local_rotation_degrees':rotations}

if __name__=='__main__':
    reports={}
    for name in ['body_dynamics_review.json','body_scenarios_review.json']:
        data=json.loads((ROOT/'Saved'/name).read_text());reports[name]=audit(data['samples'])
    passed=all(v['samples'] and not v['back_bridge_frames'] and all(
        abs(j['min'][0])<(85 if n.startswith('thigh') else 110) and abs(j['max'][0])<(85 if n.startswith('thigh') else 110) for n,j in v['joints'].items()) and all(angle<(40 if n.startswith('spine') else 75) for n,angle in v['local_rotation_degrees'].items()) for v in reports.values())
    result={'passed':passed,'reports':reports}
    (ROOT/'Saved/recovery_anatomy_review.json').write_text(json.dumps(result,indent=2))
    print(json.dumps(result,indent=2));raise SystemExit(0 if passed else 1)
