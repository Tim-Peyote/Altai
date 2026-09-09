"""Aggregate the full pose matrix and its focused rod-boundary rerun without hiding provenance."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[2]/'Saved'
full=json.loads((root/'anatomical_grips_review.json').read_text());rod=json.loads((root/'anatomical_rod_review.json').read_text())
assert not full['error'] and rod['passed']
rows=[rod['cases'][0] if r['mesh']=='SM_FieldStick' else r for r in full['cases']]
axes=[a for r in rows for a in r.get('axes',[])]
checks={'eight_cases':len(rows)==8,'48_directions':len(axes)==48,'all_held':all(r['grab'] and r['held'] and r['wrist_position_error']<2 for r in rows) and all(a['held'] for a in axes),'neutral_wrist':all(r['wrist_angular_error']<5 for r in rows),'bounded_rotation':all(a['effort']<=1.001 and a['forearm_roll']<=75.01 and a['wrist_angular_error']<8 for a in axes),'fixed_contacts':all(a['contact_slip']<.01 for a in axes),'persistent_limits':all(a['reentry_reset']<.001 for a in axes),'immediate_reverse':all(a['reverse_response']>.2 for a in axes),'steady_camera':all(r['camera_shift']<1 and r['view_restored'] for r in rows)}
result={'passed':all(checks.values()),'sources':['anatomical_grips_review.json','anatomical_rod_review.json'],'note':'The final native change removes the artificial 5-degree minimum at forearm exhaustion. The rod was the only matrix pose hitting that minimum; its six directions were rerun.','checks':checks,'max_wrist_correction_degrees':max(a['wrist_angular_error'] for a in axes),'max_camera_shift_cm':max(r['camera_shift'] for r in rows),'cases':rows}
(root/'anatomical_grips_final.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items() if k!='cases'},indent=2))
