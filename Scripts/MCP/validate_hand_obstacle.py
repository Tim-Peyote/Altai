from mcp import call
from pathlib import Path
import json,time
r={}
def f(n,a={}):
 v=call('call_tool',dict(toolset_name='altai_lab_tools.AltaiLabTools',tool_name=n,arguments=a))['result']
 if v.get('isError'):raise RuntimeError(v)
 return json.loads(json.loads(v['content'][0]['text'])['returnValue'])
r['start']=f('hand_obstacle_probe',{'prepare':True});assert r['start']['held'];time.sleep(3)
f('move_lab_player',{'seconds':1});time.sleep(1.5)
r['against_wall']=f('hand_obstacle_probe',{'prepare':False});assert r['against_wall']['held'];assert r['against_wall']['object_x']>=3570
assert r['against_wall']['pawn_x']>=3600
f('grab_lab',{'release':True});time.sleep(1)
r['released']=f('hand_obstacle_probe',{'prepare':False});assert not r['released']['held'];assert r['released']['pawn_speed']<500
f('view_lab',dict(first_person=False,pitch=-15,yaw=0));f('move_lab_player',{'seconds':1});time.sleep(2)
r['separated']=f('hand_obstacle_probe',{'prepare':False});assert 'BLOCK' in r['separated']['pawn_response']
Path(__file__).resolve().parents[2].joinpath('Saved/hand_obstacle_validation.json').write_text(json.dumps(r,indent=2))
print('HAND_OBSTACLE_VALIDATION_PASSED',r,flush=True)
