"""Independent anatomical/continuity checks on baked recovery source tracks.

Run with numpy/scipy after retargeting, before importing the assets.
These are animation acceptance bounds, not medical range-of-motion claims.
"""
import json
import numpy as np
from retarget_body_motion import ROOT, SRC, R, rig, refq, target_forward, unit

def world_pose(f):
    p, q = {}, {}
    for n, (parent, _, __) in rig.items():
        local = R.from_quat(f[n]['q']).as_matrix()
        q[n] = local if parent == 'None' else q[parent] @ local
        p[n] = np.array(f[n]['p']) if parent == 'None' else p[parent] + q[parent] @ f[n]['p']
    return p, q

def audit(label):
    data = json.loads((SRC / (label + '.json')).read_text())
    frames = data['frames']; worlds = [world_pose(f) for f in frames]
    failures = []; metrics = {}
    for n in ['pelvis', 'spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05',
              'neck_01', 'neck_02', 'head', 'thigh_l', 'thigh_r', 'upperarm_l',
              'upperarm_r', 'foot_l', 'foot_r', 'hand_l', 'hand_r']:
        q = R.from_quat([f[n]['q'] for f in frames])
        jumps = np.rad2deg((q[1:] * q[:-1].inv()).magnitude())
        metrics[n + '_max_frame_rotation'] = float(max(jumps))
        limit = 5 if n.startswith(('spine', 'foot')) else 20
        if max(jumps) > limit: failures.append(n + ': discontinuous rotation')
    for side in ['l', 'r']:
        for a, b, e in [('thigh', 'calf', 'foot'), ('upperarm', 'lowerarm', 'hand')]:
            a, b, e = [n + '_' + side for n in [a, b, e]]
            angles = []
            for p, q in worlds:
                u, v = unit(p[b]-p[a]), unit(p[e]-p[b])
                angles.append(np.rad2deg(np.arctan2(np.dot(np.cross(u,v), q[a][:,2]), np.dot(u,v))))
            metrics[b + '_flex'] = [float(min(angles)), float(max(angles))]
            if min(angles) < 1.5 or max(angles) > 140.5: failures.append(b + ': inverted/overflexed hinge')
        for name, child, limit in [('thigh','calf',80), ('upperarm','lowerarm',102), ('hand','middle_metacarpal',17)]:
            n, child = name+'_'+side, child+'_'+side; axis = unit(rig[child][1]); angles=[]
            for f in frames:
                q = (R.from_matrix(rig[n][2]).inv()*R.from_quat(f[n]['q'])).as_quat()
                angle = np.rad2deg(2*np.arctan2(np.dot(q[:3],axis),q[3])); angles.append((angle+180)%360-180)
            metrics[n+'_axial_twist'] = [float(min(angles)),float(max(angles))]
            if max(abs(np.array(angles))) > limit: failures.append(n+': excessive axial twist')
    for n in ['hand_l','hand_r','foot_l','foot_r']:
        angles=np.rad2deg((R.from_matrix(rig[n][2]).inv()*R.from_quat([f[n]['q'] for f in frames])).magnitude())
        metrics[n+'_max_local_deviation']=float(max(angles))
        if max(angles)>(71 if n.startswith('hand') else 51):failures.append(n+': excessive local rotation')
    facing = [(q['spine_05']@refq['spine_05'].T@target_forward)[2] for p,q in worlds]
    expected = 1 if label == 'GetUpSupine' else -1
    if facing[0]*expected < .8: failures.append('incorrect initial chest orientation')
    # A supine recovery must raise the head/torso before lifting the pelvis.
    # A back bridge would instead raise the pelvis while the head stays on the floor.
    if label == 'GetUpSupine':
        bridges = [i for i,(p,q) in enumerate(worlds) if p['pelvis'][2]>35 and p['head'][2]<p['pelvis'][2]+10]
        if bridges: failures.append('back bridge at frames '+str(bridges))
    metrics['initial_chest_facing_z'] = float(facing[0])
    return {'passed':not failures,'failures':failures,'metrics':metrics}

if __name__ == '__main__':
    result = {n:audit(n) for n in ['GetUpProne','GetUpSupine']}
    report = {'passed':all(v['passed'] for v in result.values()),'clips':result}
    (ROOT/'Saved/recovery_motion_review.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2))
    raise SystemExit(0 if report['passed'] else 1)
