from mcp import call
import json,time
def f(n,a={}):
 r=call('call_tool',dict(toolset_name='altai_lab_tools.AltaiLabTools',tool_name=n,arguments=a))['result']
 return json.loads(r['content'][0]['text']).get('returnValue')
print(f('prepare_hand_probe',{'mass':1}));time.sleep(.5)
f('view_lab',dict(first_person=False,pitch=-50,yaw=0));print(f('grab_lab',dict(release=False)))
for i in range(15):
 time.sleep(.15);print(f('inspect_hands'),flush=True)
