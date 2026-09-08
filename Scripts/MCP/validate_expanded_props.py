from mcp import call
from pathlib import Path
import json,time,base64,math,re
root=Path(__file__).resolve().parents[2];report={}
def f(n,a={},group='altai_lab_tools.AltaiLabTools'):
 r=call('call_tool',dict(toolset_name=group,tool_name=n,arguments=a))['result']
 if r.get('isError'):raise RuntimeError(r)
 v=json.loads(r['content'][0]['text']).get('returnValue')
 if isinstance(v,str):
  try:return json.loads(v)
  except ValueError:return v
 return v
def capture(name):
 v=f('CaptureEditorImage',{},'EditorToolset.EditorAppToolset');(root/'docs/Images'/('expanded_'+name+'.png')).write_bytes(base64.b64decode(v['data']))
time.sleep(2);report['initial']=f('inspect_load')
assert len(report['initial']['props'])>=8
for name,mass in [('Loose_Stone_1kg',1),('Loose_Stone_5kg',5),('Loose_Stone_20kg',20),('Carry_Bucket_Empty_2kg',2),('Carry_Bucket_Ballast_12kg',12),('Carry_Stick_08kg',.8),('Carry_Crate_18kg',18),('Carry_Crate_40kg',40)]:
 prep=f('prepare_station_probe',{'name':name});time.sleep(.6)
 def vector(s):return [float(x) for x in re.findall(r'[xyz]: ([+-]?[0-9.]+)',s)]
 p=vector(f('inspect_lab')['position']);c=vector(prep['center']);pitch=math.degrees(math.atan2(c[2]-p[2]-55,math.hypot(c[0]-p[0],c[1]-p[1])))
 f('view_lab',dict(first_person=False,pitch=pitch,yaw=180));v=f('grab_lab',{'release':False});print('grab',name,v,flush=True)
 time.sleep(3);v=f('inspect_load');report[name]=v;(root/'Saved/expanded_props_validation.json').write_text(json.dumps(report,indent=2))
 if mass>30:assert not v['held'];continue
 assert v['held'],v
 assert abs(float(v['held_mass'])-mass)<.1
 assert v['angular_speed']<1,v
 assert v['contact_errors'] and max(v['contact_errors'].values())<8,v
 assert v['speed']<500 and v['acceleration']<2048,v
 f('view_lab',dict(first_person=True,pitch=-55,yaw=180));time.sleep(.5);capture(name+'_first')
 f('view_lab',dict(first_person=False,pitch=-30,yaw=180));time.sleep(.5);capture(name+'_third')
 f('grab_lab',{'release':True});time.sleep(.8);after=f('inspect_load');assert not after['held'] and abs(after['speed']-500)<1,after
report['final']=f('inspect_load')
start=next(a for a in report['initial']['props'] if a['name']=='Carry_Stick_08kg');end=next(a for a in report['final']['props'] if a['name']=='Carry_Stick_08kg')
a=vector(start['position']);b=vector(end['position']);report['released_stick_drift_cm']=math.hypot(a[0]-b[0],a[1]-b[1]);assert report['released_stick_drift_cm']<250,report['released_stick_drift_cm']
(root/'Saved/expanded_props_validation.json').write_text(json.dumps(report,indent=2));print('EXPANDED_PROPS_VALIDATION_PASSED',flush=True)
