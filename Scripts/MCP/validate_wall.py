from mcp import call
from pathlib import Path
import json,time,base64
root=Path(__file__).resolve().parents[2];r={}
def invoke(name,args={},group='altai_lab_tools.AltaiLabTools'):
 v=call('call_tool',dict(toolset_name=group,tool_name=name,arguments=args))['result']
 if v.get('isError'):raise RuntimeError(v)
 v=json.loads(v['content'][0]['text'])['returnValue']
 if isinstance(v,str):
  try:return json.loads(v)
  except ValueError:pass
 return v
def wall(action='inspect'):return invoke('wall_lab',{'action':action})
def capture(name):
 v=invoke('CaptureEditorImage',{},'EditorToolset.EditorAppToolset')
 (root/'docs/Images'/('wall_'+name+'.png')).write_bytes(base64.b64decode(v['data']))
for kind in ['RoughRock','SmoothRock']:
 invoke('prepare_wall_probe',{'kind':kind});time.sleep(.6)
 assert wall('attach')['attached']=='True'
 invoke('view_lab',dict(first_person=False,pitch=-10,yaw=180));time.sleep(1)
 r[kind+'_dry']=wall();r[kind+'_contacts']=invoke('inspect_hands')
 print(kind,r[kind+'_dry'],r[kind+'_contacts'],flush=True)
 for i in range(35):wall('up');time.sleep(.06)
 time.sleep(.3);capture(kind)
 assert wall()['attached']=='True'
 # The shared hands component must refuse an object while supporting the wall.
 assert invoke('grab_lab',{'release':False})['held'] is None
 invoke('view_lab',dict(first_person=True,pitch=-45,yaw=180));time.sleep(.6);assert wall()['attached']=='True';capture(kind+'_first')
 invoke('set_lab_weather',dict(preset=2,hour=15));time.sleep(6)
 r[kind+'_wet']=wall();print(kind,'wet',r[kind+'_wet'],flush=True)
 assert float(r[kind+'_wet']['wetness'])>0
 assert float(r[kind+'_wet']['grip'])<float(r[kind+'_dry']['grip'])
 wall('release');time.sleep(1);assert wall()['attached']=='False'
 invoke('set_lab_weather',dict(preset=1,hour=15))
(root/'Saved/wall_validation.json').write_text(json.dumps(r,indent=2))
print('WALL_VALIDATION_PASSED',flush=True)
