"""Exercise the actual low-obstacle trace at walking/running speed, not debug trip calls."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);body=pc.body_dynamics
unreal.AltaiEditorLibrary.set_physics_test_mode(True);p.set_first_person(False);pc.hands.release()
actors={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor)}
items=[actors['Throw_Target_1kg'],actors['Throw_Bottle_1kg']];saved=[]
for a in items:
 c=a.static_mesh_component;saved.append((a.get_actor_transform(),c.static_mesh,c.is_simulating_physics(),c.get_collision_profile_name()));c.set_simulate_physics(False);c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));c.set_collision_profile_name('BlockAll')
items[0].set_actor_scale3d(unreal.Vector(60,60,.2));items[0].set_actor_location(unreal.Vector(9000,-1800,3390),False,True)
old=p.get_actor_transform();rows=[]
def wait(s):
 end=time.monotonic()+s
 while time.monotonic()<end:yield

def cases():
 for height in [20,50]:
  items[1].set_actor_scale3d(unreal.Vector(.3,3,height/100));items[1].set_actor_location(unreal.Vector(9350,-1800,3400+height/2),False,True)
  p.set_actor_location(unreal.Vector(9000,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True);p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING);p.character_movement.stop_movement_immediately();yield from wait(1.4)
  before=body.falls;recover=body.recoveries;steps=pc.surface_response.stumble_count;end=time.monotonic()+2;peak=0
  while time.monotonic()<end:
   p.add_movement_input(unreal.Vector(1,0,0),1,False);peak=max(peak,body.reaction);yield
  p.character_movement.stop_movement_immediately();yield from wait(.3)
  end=time.monotonic()+15
  while body.state!=unreal.AltaiBodyState.BALANCED and time.monotonic()<end:yield
  tripped=pc.surface_response.stumble_count>steps;fell=body.falls>before
  rows.append({'height':height,'tripped':tripped,'fell':fell,'recovered':body.recoveries>recover,'peak_reaction':peak,'hint':body.hint,'passed':tripped and (not fell if height==20 else fell and body.recoveries>recover)})

def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 if body.state==unreal.AltaiBodyState.BALANCED:
  p.set_actor_transform(old,False,True)
  for a,s in zip(items,saved):
   c=a.static_mesh_component;c.set_simulate_physics(False);c.set_static_mesh(s[1]);a.set_actor_transform(s[0],False,True);c.set_collision_profile_name(s[3]);c.set_simulate_physics(s[2])
 Path(unreal.Paths.project_saved_dir(),'trip_obstacles_review.json').write_text(json.dumps({'passed':not error and len(rows)==2 and all(r['passed'] for r in rows),'cases':rows,'error':error},indent=2))
g=cases()
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
