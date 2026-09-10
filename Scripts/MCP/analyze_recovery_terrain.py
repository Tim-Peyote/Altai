"""Check actual planted contact errors and proximal joints in terrain integration data."""
import json,math
from analyze_recovery_anatomy import audit,ROOT

p=ROOT/'Saved/recovery_terrain_review.json';data=json.loads(p.read_text())
anatomy=audit(data['samples']);contacts={}
for row in data['samples']:
    if not .2<row['progress']<.9:continue
    for i,name in enumerate(['hand_l','hand_r','foot_l','foot_r']):
        if i>=len(row['contacts']):continue
        c=row['contacts'][i]
        if c['weight']<.95:continue
        error=math.dist(c['goal'],row['bones'][name]);key=row['case']+':'+name
        contacts[key]=max(contacts.get(key,0),error)
safe=not anatomy['back_bridge_frames'] and all(j['min'] and abs(j['min'][0])<110 and abs(j['max'][0])<110 for j in anatomy['joints'].values())
result={'passed':data['passed'] and safe and bool(contacts) and max(contacts.values())<8,
        'integration_passed':data['passed'],'anatomy':anatomy,'max_planted_contact_error_cm':contacts}
(ROOT/'Saved/recovery_terrain_analysis.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
