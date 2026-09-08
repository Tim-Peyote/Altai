"""Exercise physical handles, not component transforms: closed -> open -> release -> close."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.hands;h.release()
props=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.AltaiArticulatedProp)
p.set_actor_location(unreal.Vector(5200,-5600,248),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=165,roll=0));p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=165,roll=0),True)
rows=[];start=time.monotonic();phase=-1
report={'initial':{a.get_actor_label():a.get_opening() for a in props}}
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);report['samples']=rows;report['error']=error
 report['passed']=not error and all(abs(v)<3 for v in report['initial'].values()) and all(abs(v['open']-v['travel'])<8 and abs(v['closed'])<5 for v in report.get('results',{}).values())
 Path(unreal.Paths.project_saved_dir(),'furniture_joint_validation.json').write_text(json.dumps(report,indent=2));unreal.log('FURNITURE_JOINT_VALIDATION '+str(report['passed']))
def tick(dt):
 global phase
 try:
  t=time.monotonic()-start;newphase=0 if t<6 else 1 if t<8 else 2 if t<14 else 3
  if newphase!=phase:
   if newphase==1:report['results']={a.get_actor_label():{'open':a.get_opening(),'travel':a.travel} for a in props}
   if newphase==2:
    for a in props:report['results'][a.get_actor_label()]['released']=a.get_opening()
   if newphase==3:
    for a in props:report['results'][a.get_actor_label()]['closed']=a.get_opening()
    finish();return
   phase=newphase
  for a in props:
   if phase!=1:a.drive_grip(a.travel if phase==0 else 0,12000)
  rows.append({'t':t,'phase':phase,'opening':{a.get_actor_label():a.get_opening() for a in props}})
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
