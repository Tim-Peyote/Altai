from mcp import call
from pathlib import Path
import json,time,base64
root=Path(__file__).resolve().parents[2];report={}
def f(n,a={},group='altai_lab_tools.AltaiLabTools'):
 r=call('call_tool',dict(toolset_name=group,tool_name=n,arguments=a))['result']
 if r.get('isError'):raise RuntimeError(r)
 v=json.loads(r['content'][0]['text']).get('returnValue')
 if isinstance(v,str):
  try:return json.loads(v)
  except ValueError:return v
 return v
f('set_lab_weather',{'preset':1,'hour':14})
for kind in ['RoughRock','SmoothRock','MossyRock']:
 for z in [0,230,650,880,1350]:
  start=f('prepare_wall_height',{'kind':kind,'height_cm':z});assert start['attached'],start;time.sleep(.5)
  v=f('inspect_wall_contacts');report[kind+'_'+str(z)]=v;print(kind,z,v,flush=True)
  assert v['attached']=='True' and len(v['contacts'])==4 and max(v['contacts'].values())<5,v
  assert start['wall_height']>=1600
  f('wall_lab',{'action':'release'})
# Rain changes wetness on tall exposed walls despite their rock crowns.
f('prepare_wall_height',{'kind':'SmoothRock','height_cm':650});time.sleep(.3);dry=f('inspect_wall_contacts');f('set_lab_weather',{'preset':2,'hour':14});time.sleep(6);wet=f('inspect_wall_contacts');report['rain']={'dry':dry,'wet':wet};assert float(wet['wetness'])>float(dry['wetness']) and float(wet['grip'])<float(dry['grip'])
f('wall_lab',{'action':'release'});(root/'Saved/expanded_wall_validation.json').write_text(json.dumps(report,indent=2));print('EXPANDED_WALL_VALIDATION_PASSED',flush=True)
