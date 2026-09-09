"""PIE integration review of real rigid bodies, reachable contacts and throw modes."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.hands
h.release();p.un_crouch();p.set_first_person(True);unreal.AltaiEditorLibrary.set_physics_test_mode(True)
old_location=p.get_actor_location();old_rotation=pc.get_control_rotation();created=[];rows=[]
actors={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor)}
snapshots={}
def spawn(mesh,pos,mass=None,scale=None):
 a=actors['Throw_Bottle_1kg' if mass else 'Throw_Target_1kg'];c=a.static_mesh_component
 if a not in snapshots:snapshots[a]=(a.get_actor_transform(),c.static_mesh,c.get_mass(),c.is_simulating_physics())
 c.set_simulate_physics(False);c.set_static_mesh(unreal.load_asset(mesh));c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_collision_profile_name('PhysicsActor' if mass else 'BlockAll');a.set_actor_scale3d(unreal.Vector(*(scale or (1,1,1))));a.set_actor_location(unreal.Vector(*pos),False,True);a.set_actor_rotation(unreal.Rotator(),True)
 if mass:c.set_mass_override_in_kg('None',mass,True);c.set_simulate_physics(True);c.set_physics_linear_velocity(unreal.Vector());c.set_physics_angular_velocity_in_degrees(unreal.Vector())
 return c
floor=spawn('/Engine/BasicShapes/Cube',(6400,-1800,3390),scale=(12,12,.2))
def pause(seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:yield

def flight():
 wall_actor=actors['Throw_Target_3kg'];wall=wall_actor.static_mesh_component
 snapshots[wall_actor]=(wall_actor.get_actor_transform(),wall.static_mesh,wall.get_mass(),wall.is_simulating_physics())
 wall.set_simulate_physics(False);wall.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));wall.set_collision_profile_name('BlockAll');wall_actor.set_actor_scale3d(unreal.Vector(.02,4,5));wall_actor.set_actor_location(unreal.Vector(6480,-1800,3600),False,True);wall_actor.set_actor_rotation(unreal.Rotator(),True)
 p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(6300,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True)
 c=spawn('/Game/Altai/Environment/Geometry/SM_HandFlask',(6360,-1800,3530),.25);c.set_enable_gravity(False)
 yield from pause(.4);pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),c.get_center_of_mass()));yield from pause(.2)
 row={'grab':h.try_grab(False)};rows.append(row);c.set_enable_gravity(True);yield from pause(1.6)
 pc.set_control_rotation(unreal.Rotator());row['charge']=h.begin_charge_throw();yield from pause(1.3);h.release_charged_throw();row['max_x']=0;row['collision_response']=[];end=time.monotonic()+3
 while time.monotonic()<end:
  pos=c.get_center_of_mass();row['max_x']=max(row['max_x'],pos.x);row['collision_response'].append(str(c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)));yield
 row['speed']=h.last_throw_speed;row['final_speed']=c.get_physics_linear_velocity().length();row['blocked_by_thin_wall']=row['max_x']<6481 and row['max_x']>6460;row['pawn_collision_restored']='BLOCK' in row['collision_response'][-1]
 row['collision_response']=sorted(set(row['collision_response']))
g=flight()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);h.release()
 for a,(t,mesh,mass,sim) in snapshots.items():
  c=a.static_mesh_component;c.set_simulate_physics(False);c.set_static_mesh(mesh);a.set_actor_transform(t,False,True);c.set_mass_override_in_kg('None',mass,True);c.set_enable_gravity(True);c.set_simulate_physics(sim)
 p.set_actor_location(old_location,False,True);pc.set_control_rotation(old_rotation);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 Path(unreal.Paths.project_saved_dir(),'object_collision_review.json').write_text(json.dumps({'passed':not error and bool(rows) and rows[0].get('blocked_by_thin_wall') and rows[0].get('pawn_collision_restored'),'error':error,'cases':rows},indent=2));unreal.log('ALTAI_COLLISION_REVIEW_DONE '+error)
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
