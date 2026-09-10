"""PIE integration on the saved pond: entry, sprint, fatigue, dive, ascend, exit."""
import unreal,json,time,math,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);s=pc.swimming
s.input_from_player=False;s.sprint=s.dive=s.ascend=False;p.set_first_person(False)
old=p.get_actor_transform();cases=[];samples=[];label='';names=['pelvis','spine_01','spine_02','spine_03','spine_04','spine_05','neck_01','neck_02','head','clavicle_l','clavicle_r','upperarm_l','lowerarm_l','hand_l','upperarm_r','lowerarm_r','hand_r','thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r']
def wait(t,direction=None):
 end=unreal.GameplayStatics.get_time_seconds(w)+t
 while unreal.GameplayStatics.get_time_seconds(w)<end:
  if direction:p.add_movement_input(unreal.Vector(*direction),1,False)
  yield

def run():
 global label
 p.set_actor_location(unreal.Vector(3100,1700,105),False,True);p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FALLING);pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=180,roll=0))
 label='wading';yield from wait(1);shallow=s.depth;slow=p.character_movement.max_walk_speed;yield from wait(2,(-1,0,0))
 cases.append({'case':'wading','passed':shallow>5 and any(r['state']=='WADING' for r in samples),'initial_depth':shallow,'initial_walk_speed':slow})
 label='swim';p.set_actor_location(unreal.Vector(2000,1700,0),False,True);yield from wait(2,(0,1,0));normal=s.speed
 cases.append({'case':'surface','passed':s.state==unreal.AltaiSwimState.SURFACE,'speed':normal})
 label='sprint';s.sprint=True;yield from wait(1.5,(1,0,0));fast=s.speed
 cases.append({'case':'sprint','passed':fast>normal+15,'speed':fast})
 label='fatigue';s.set_test_stamina(.1);yield from wait(2,(-1,0,0));cases.append({'case':'fatigue','passed':s.fatigue>.8 and s.speed<130,'speed':s.speed,'fatigue':s.fatigue})
 label='dive';s.sprint=False;s.set_test_stamina(1);p.set_actor_location(unreal.Vector(1800,1700,-10),False,True);p.set_first_person(True);s.dive=True;yield from wait(2);s.dive=False;yield from wait(.5)
 cases.append({'case':'dive','passed':s.state==unreal.AltaiSwimState.DIVING and p.get_actor_location().z<-90 and s.camera_underwater>.8,'z':p.get_actor_location().z,'post_process':s.camera_underwater})
 label='underwater_direction';p.set_actor_location(unreal.Vector(1800,1700,-150),False,True);p.character_movement.stop_movement_immediately();before=p.get_actor_location();yield from wait(1,(0,.8660254,-.5));delta=p.get_actor_location()-before
 cases.append({'case':'underwater_direction','passed':delta.y>30 and delta.z<-15 and s.speed<=176,'delta':list(delta.to_tuple()),'speed':s.speed})
 label='ascend';s.ascend=True;yield from wait(4);s.ascend=False;yield from wait(.5)
 cases.append({'case':'ascend','passed':s.state==unreal.AltaiSwimState.SURFACE,'z':p.get_actor_location().z})
 label='exit';p.set_first_person(False);end=unreal.GameplayStatics.get_time_seconds(w)+12
 while unreal.GameplayStatics.get_time_seconds(w)<end:
  p.add_movement_input(unreal.Vector(1,0,0),1,False)
  if s.state==unreal.AltaiSwimState.DRY and p.character_movement.is_moving_on_ground():break
  yield
 p.character_movement.stop_movement_immediately()
 cases.append({'case':'exit','passed':s.state==unreal.AltaiSwimState.DRY and p.character_movement.is_moving_on_ground(),'state':str(s.state)})
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);s.input_from_player=True;s.sprint=s.dive=s.ascend=False
 if s.state==unreal.AltaiSwimState.DRY:p.set_actor_transform(old,False,True)
 Path(unreal.Paths.project_saved_dir(),'swimming_review.json').write_text(json.dumps({'passed':not error and len(cases)==8 and all(c['passed'] for c in cases),'cases':cases,'error':error,'samples':samples},indent=2))
g=run();wall_start=time.monotonic()
def tick(dt):
 try:
  if time.monotonic()-wall_start>120:raise RuntimeError("PIE simulation did not complete in 120 wall seconds")
  next(g)
  samples.append({'case':label,'state':str(s.state).split('.')[1].split(':')[0],'alpha':s.swim_alpha,'travel_blend':s.travel_blend,'wade':s.wade_alpha,'phase':s.phase,'depth':s.depth,'stamina':s.stamina,'fatigue':s.fatigue,'position':list(p.get_actor_location().to_tuple()),'camera':list(pc.player_camera_manager.get_camera_location().to_tuple()),'bones':{n:list(p.mesh.get_socket_location(n).to_tuple()) for n in names},'rotations':{n:list(p.mesh.get_socket_rotation(n).quaternion().to_tuple()) for n in names}})
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
