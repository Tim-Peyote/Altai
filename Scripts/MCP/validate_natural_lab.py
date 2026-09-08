"""PIE integration checks using the project's bounded, lab-only tools."""
from mcp import call
from pathlib import Path
import json,time,hashlib,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Editor'));from lab_landform import height
root=Path(__file__).resolve().parents[2];report={}
def invoke(tool,args={},group='altai_lab_tools.AltaiLabTools'):
 r=call('call_tool',{'toolset_name':group,'tool_name':tool,'arguments':args})
 data=r.get('result',r)
 if data.get('isError'):raise RuntimeError(data)
 text=data['content'][0]['text'];v=json.loads(text).get('returnValue')
 if isinstance(v,str):
  try:return json.loads(v)
  except ValueError:return v
 return v
def inspect(name):
 v=invoke('inspect_lab');report[name]=v;print(name,json.dumps(v),flush=True);return v
def place(x,y,yaw=0):
 invoke('place_lab_player',{'x':x,'y':y,'z':height(x,y)+180,'yaw':yaw});time.sleep(2)
def saves():return {str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'Saved/SaveGames').rglob('*') if p.is_file()}
report['saves_before']=saves()
inspect('spawn')
place(1800,1700);v=inspect('water');assert abs(v['max_speed']-275)<2
invoke('move_lab_player',{'seconds':2});time.sleep(3);inspect('water_walk')
place(-700,1300);v=inspect('mud');assert abs(v['max_speed']-190)<2
invoke('move_lab_player',{'seconds':2});time.sleep(3);v=inspect('mud_walk');assert v['decals']>0
place(1000,-3100);v=inspect('dry');assert abs(v['max_speed']-500)<2
place(-2600,-2200);v=inspect('snow_patch');assert 'SNOW' in v['surface']['current_surface']
invoke('move_lab_player',{'seconds':2});time.sleep(3);inspect('snow_walk')
place(3300,-2400,90);invoke('move_lab_player',{'seconds':2});time.sleep(3);v=inspect('obstacle');assert int(v['surface']['stumble_count'])>0
for i in range(7):
 invoke('set_lab_weather',{'preset':i,'hour':14});time.sleep(.4);v=inspect('preset_'+str(i));assert v['weather']['preset_index']==i
invoke('set_lab_weather',{'preset':3,'hour':23});inspect('night')
invoke('set_lab_automation',{'clock':True,'weather':True});time.sleep(3);v=inspect('clock');assert v['weather']['hour']>23
invoke('reset_lab');time.sleep(4);v=inspect('reset');assert v['surface']['step_count']=='0';assert v['weather']['preset_index']==1
report['saves_after']=saves();assert report['saves_before']==report['saves_after'];report['passed']=True
(root/'Saved/natural_lab_validation.json').write_text(json.dumps(report,indent=2));print('NATURAL_LAB_VALIDATION_PASSED')
