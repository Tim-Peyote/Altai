"""Actual PIE camera, natural ledge and surface regression, with reviewable captures."""
from mcp import call
from pathlib import Path
import json,time,base64,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Editor'))
from lab_landform import height
root=Path(__file__).resolve().parents[2];report={}
def invoke(name,args={},group='altai_lab_tools.AltaiLabTools'):
 r=call('call_tool',dict(toolset_name=group,tool_name=name,arguments=args))['result']
 if r.get('isError'):raise RuntimeError(r)
 v=json.loads(r['content'][0]['text'])['returnValue']
 if isinstance(v,str):
  try:return json.loads(v)
  except ValueError:pass
 return v
def snap(name):
 v=invoke('inspect_lab');report[name]=v;print(name,json.dumps(v),flush=True);return v
def capture(name):
 v=invoke('CaptureEditorImage',{},'EditorToolset.EditorAppToolset')
 p=root/'docs/Images'/('interaction_'+name+'.png');p.write_bytes(base64.b64decode(v['data']))
def place(x,y,yaw=0):
 invoke('place_lab_player',dict(x=x,y=y,z=height(x,y)+180,yaw=yaw));time.sleep(1.4)
place(-2760,350,180)
v=snap('natural_ledge');assert v['traversal']['can_climb']=='True'
invoke('climb_lab',{'cancel':False});time.sleep(1.6)
v=snap('climb_completed');assert int(v['traversal']['completed_climbs'])==1;assert 'WALKING' in v['movement_mode']
capture('ledge')
place(-2760,350,180);invoke('climb_lab',{'cancel':False});time.sleep(.2);invoke('climb_lab',{'cancel':True});time.sleep(1)
v=snap('climb_cancelled');assert v['traversal']['climbing']=='False';assert 'WALKING' in v['movement_mode']
place(4700,-3100,0);invoke('climb_lab',{'cancel':False});assert snap('no_ledge')['traversal']['climbing']=='False'
for name,x,y in [('snow',-2750,-2200),('mud',-900,1300),('water',1700,1700)]:
 place(x,y)
 invoke('view_lab',dict(first_person=True,pitch=-55,yaw=0));time.sleep(.7)
 v=snap(name+'_first_person');assert v['first_person']
 # Move while looking down; camera aim must not affect production WASD input.
 invoke('move_lab_player',{'seconds':2.5});time.sleep(1.5);capture(name+'_active');time.sleep(1.4)
 v=snap(name+'_walk')
 invoke('view_lab',dict(first_person=False,pitch=-60,yaw=0));time.sleep(.7);capture(name+'_tracks')
 assert not snap(name+'_third_person')['first_person']
report['passed']=True
(root/'Saved/interaction_validation.json').write_text(json.dumps(report,indent=2))
print('INTERACTION_VALIDATION_PASSED',flush=True)
