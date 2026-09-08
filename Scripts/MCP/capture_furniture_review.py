from furniture_console import f,run,ROOT
import time,json,base64
APP='EditorToolset.EditorAppToolset';images=ROOT/'docs/Images';images.mkdir(exist_ok=True)
def capture(name):
 v=f('CaptureEditorImage',{},APP);(images/(name+'.png')).write_bytes(base64.b64decode(v['data']))
def play():f('StartPIE',{'options':{'bSimulate':False,'playMode':'PlayMode_InViewPort','warmupSeconds':1}},APP)
for name,actor,fp in [('furniture_drawer_fp','Interact_Drawer_2',True),('furniture_chest_fp','Interact_Chest',True),('furniture_overview','Interact_Drawer_2',False)]:
 if f('IsPIERunning',{},APP):f('StopPIE',{},APP)
 play();(ROOT/'Saved/furniture_pose_request.json').write_text(json.dumps({'name':actor,'first_person':fp}));run('prepare_furniture_pose.py');time.sleep(7);capture(name);print('CAPTURED',name,flush=True)
f('StopPIE',{},APP)
