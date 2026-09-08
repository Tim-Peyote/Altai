from mcp import call
from pathlib import Path
import json,time,base64
root=Path(__file__).resolve().parents[2];report={}
def invoke(name,args={},group='altai_lab_tools.AltaiLabTools'):
 r=call('call_tool',dict(toolset_name=group,tool_name=name,arguments=args))['result']
 if r.get('isError'):raise RuntimeError(r)
 v=json.loads(r['content'][0]['text']).get('returnValue')
 if isinstance(v,str):
  try:return json.loads(v)
  except ValueError:pass
 return v
def capture(name):
 v=invoke('CaptureEditorImage',{},'EditorToolset.EditorAppToolset')
 (root/'docs/Images'/('hands_'+name+'.png')).write_bytes(base64.b64decode(v['data']))
for m in [1,5,20]:
 invoke('prepare_hand_probe',{'mass':m});time.sleep(.8)
 invoke('view_lab',dict(first_person=False,pitch=-50,yaw=0));
 print('grab',m,invoke('grab_lab',{'release':False}),flush=True)
 time.sleep(4)
 v=invoke('inspect_hands');report[str(m)]=v;print(m,json.dumps(v),flush=True)
 capture(str(m)+'_third')
 assert v['held'],v
 assert v['contact_errors'] and max(v['contact_errors'].values())<5,v
 assert abs(float(v['held_mass'])-m)<.2
 assert float(v['applied_force'])<=(32001 if m>3 else 12001)
 invoke('view_lab',dict(first_person=True,pitch=-60,yaw=0));time.sleep(.8);capture(str(m)+'_first')
 invoke('grab_lab',{'release':True});time.sleep(.5);assert invoke('inspect_hands')['held'] is None
 invoke('view_lab',dict(first_person=False,pitch=-43,yaw=0));time.sleep(.5)
(root/'Saved/hands_validation.json').write_text(json.dumps(report,indent=2))
print('HANDS_VALIDATION_PASSED',flush=True)
