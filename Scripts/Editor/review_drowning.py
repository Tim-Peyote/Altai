"""PIE oxygen, rescue, terminal drowning and debug reset with actual pose samples."""
import unreal,json,time,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);s=pc.swimming
s.reset_at_shore();s.input_from_player=False;p.set_first_person(False)
cases=[];samples=[];label='';names=['upperarm_l','lowerarm_l','hand_l','upperarm_r','lowerarm_r','hand_r','thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r','head','pelvis']
def wait(seconds,direction=None):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if direction:p.add_movement_input(unreal.Vector(*direction),1,False)
  yield

def enter():
 p.set_actor_location(unreal.Vector(1800,1700,-150),False,True);p.character_movement.stop_movement_immediately();s.dive=True
 yield from wait(.4)
 s.dive=False

def run():
 global label
 label='oxygen_drain';yield from enter();a=s.oxygen;yield from wait(1)
 cases.append({'case':label,'passed':s.airway_underwater and .018<a-s.oxygen<.05,'oxygen_delta':a-s.oxygen})
 label='rescue';s.set_test_oxygen(0);yield from wait(1.5);h=s.drowning_health;b=s.blackout
 cases.append({'case':'suffocation','passed':not s.dead and .5<h<.95 and b>.1,'health':h,'blackout':b})
 p.set_actor_location(unreal.Vector(1800,1700,3),False,True);p.character_movement.stop_movement_immediately();yield from wait(2)
 cases.append({'case':label,'passed':not s.dead and s.oxygen>.25 and s.drowning_health>h and s.blackout<b,'oxygen':s.oxygen,'health':s.drowning_health,'blackout':s.blackout})
 label='death';yield from enter();s.set_test_oxygen(0);s.dive=False;yield from wait(6.5)
 cases.append({'case':label,'passed':s.dead and s.drowning_health==0 and p.actor_has_tag('AltaiDead'),'progress':s.death_progress})
 label='terminal';start=p.get_actor_location();s.set_test_oxygen(1);s.dive=False;s.ascend=True;yield from wait(4.5,(1,0,1));end=p.get_actor_location()
 cases.append({'case':label,'passed':s.dead and s.oxygen==0 and s.blackout>.99 and s.death_progress==1 and abs(end.x-start.x)<3 and end.z<start.z-25,'blackout':s.blackout,'delta':list((end-start).to_tuple())})
 label='reset';s.reset_at_shore();yield from wait(.5)
 cases.append({'case':label,'passed':not s.dead and not p.actor_has_tag('AltaiDead') and s.oxygen>.99 and s.drowning_health==1 and s.blackout==0 and s.state in [unreal.AltaiSwimState.DRY,unreal.AltaiSwimState.WADING],'state':str(s.state),'oxygen':s.oxygen,'blackout':s.blackout})

def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);s.reset_at_shore();s.input_from_player=True
 Path(unreal.Paths.project_saved_dir(),'drowning_review.json').write_text(json.dumps({'passed':not error and len(cases)==6 and all(c['passed'] for c in cases),'cases':cases,'error':error,'samples':samples},indent=2))
g=run()
def tick(dt):
 try:
  next(g)
  samples.append({'case':label,'dead':s.dead,'death_progress':s.death_progress,'oxygen':s.oxygen,'health':s.drowning_health,'blackout':s.blackout,'bones':{n:list(p.mesh.get_socket_location(n).to_tuple()) for n in names},'rotations':{n:list(p.mesh.get_socket_rotation(n).quaternion().to_tuple()) for n in names}})
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
