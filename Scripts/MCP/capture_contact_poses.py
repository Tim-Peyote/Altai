"""Capture evaluated support poses in the actual lab, after motion regression."""
from mcp import call
from pathlib import Path
import json,time,base64
root=Path(__file__).resolve().parents[2]
def f(n,a={},group='altai_lab_tools.AltaiLabTools'):
 r=call('call_tool',dict(toolset_name=group,tool_name=n,arguments=a))['result']
 if r.get('isError'):raise RuntimeError(r)
 v=json.loads(r['content'][0]['text']).get('returnValue')
 if isinstance(v,str):
  try:return json.loads(v)
  except ValueError:return v
 return v
def capture(name):
 time.sleep(.8);v=f('CaptureEditorImage',{},'EditorToolset.EditorAppToolset');(root/'docs/Images'/('contact_'+name+'.png')).write_bytes(base64.b64decode(v['data']))
def select(i):
 for k in range(4):
  if int(f('wall_lab',{'action':'inspect'})['selected_limb'])==i:return
  f('wall_lab',{'action':'next'})
f('set_lab_weather',{'preset':1,'hour':14});f('prepare_wall_height',{'kind':'RoughRock','height_cm':650});f('view_lab',{'first_person':False,'pitch':-12,'yaw':165});capture('four_supports')
select(2);f('wall_lab',{'action':'free'});select(3);f('wall_lab',{'action':'free'});capture('two_hands')
select(1);f('wall_lab',{'action':'free'});capture('one_hand')
f('wall_lab',{'action':'release'});f('prepare_wall_height',{'kind':'RoughRock','height_cm':650});select(1);f('wall_lab',{'action':'free'});select(2);f('wall_lab',{'action':'free'});capture('diagonal_support')
f('view_lab',{'first_person':True,'pitch':-18,'yaw':180});capture('first_person_wall')
f('wall_lab',{'action':'release'});f('place_lab_player',{'x':4700,'y':-3100,'z':160,'yaw':0});print('CONTACT_POSES_CAPTURED')
