from mcp import call
from pathlib import Path
import json,time,math,re
root=Path(__file__).resolve().parents[2]
def f(n,a={}):
 r=call('call_tool',dict(toolset_name='altai_lab_tools.AltaiLabTools',tool_name=n,arguments=a))['result']
 if r.get('isError'):raise RuntimeError(r)
 v=json.loads(r['content'][0]['text'])['returnValue']
 try:return json.loads(v)
 except (ValueError,TypeError):return v
prep=f('prepare_station_probe',{'name':'Loose_Stone_20kg'});time.sleep(.8)
def vec(s):return [float(x) for x in re.findall(r'[xyz]: ([+-]?[0-9.]+)',s)]
p=vec(f('inspect_lab')['position']);c=vec(prep['center']);f('view_lab',dict(first_person=False,pitch=math.degrees(math.atan2(c[2]-p[2]-55,abs(c[0]-p[0]))),yaw=180));f('grab_lab',{'release':False});time.sleep(3);assert float(f('inspect_load')['held_mass'])==20
r={}
for i in range(4):
 f('cycle_lab_body_mass');time.sleep(.8);s=f('inspect_load');r[str(round(s['body_mass']))]=s;assert abs(s['speed']-500/(1+40/s['body_mass']))<1
assert r['60']['speed']<r['120']['speed'];assert r['60']['acceleration']<r['120']['acceleration']
f('grab_lab',{'release':True});time.sleep(1);s=f('inspect_load');assert abs(s['speed']-500)<1 and s['acceleration']==2048
(root/'Saved/body_load_validation.json').write_text(json.dumps(r,indent=2));print('BODY_LOAD_VALIDATION_PASSED',flush=True)
