from mcp import call
from pathlib import Path
import json,base64,time
def invoke(name,args={},group='altai_lab_tools.AltaiLabTools'):
 r=call('call_tool',dict(toolset_name=group,tool_name=name,arguments=args))['result']
 if r.get('isError'):raise RuntimeError(r)
 return json.loads(r['content'][0]['text'])['returnValue']
def capture(name):
 v=invoke('CaptureEditorImage',{},'EditorToolset.EditorAppToolset')
 p=Path(__file__).resolve().parents[2]/'docs/Images'/('interaction_'+name+'.png')
 p.write_bytes(base64.b64decode(v['data']))
for name,x,y,z in [('water',1700,1700,130),('mud',-900,1300,200),('snow',-2750,-2200,250)]:
 invoke('place_lab_player',dict(x=x,y=y,z=z,yaw=0));time.sleep(1)
 invoke('view_lab',dict(first_person=False,pitch=-65,yaw=0));time.sleep(.7)
 invoke('move_lab_player',{'seconds':3});time.sleep(1.7);capture(name+'_motion');time.sleep(1.8)
invoke('view_lab',dict(first_person=True,pitch=-85,yaw=0));time.sleep(.8);capture('first_person_down')
print('MOTION_CAPTURES_SAVED')
