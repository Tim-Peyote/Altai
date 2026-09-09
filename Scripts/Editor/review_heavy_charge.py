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

def cases():
 p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(6300,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True);pc.set_control_rotation(unreal.Rotator())
 c=spawn('/Game/Altai/Environment/Geometry/SM_FieldStone',(6360,-1800,3530),20);c.set_enable_gravity(False);yield from pause(.5)
 for k in range(10):pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),c.get_center_of_mass()));yield
 h.try_grab(False);c.set_enable_gravity(True);yield from pause(1.8);pc.set_control_rotation(unreal.Rotator(pitch=15,yaw=0,roll=0));h.begin_charge_throw();end=time.monotonic()+2;start=time.monotonic()
 while time.monotonic()<end:
  rows.append({'t':time.monotonic()-start,'held':h.held==c,'hint':h.hint,'wrist_angle':h.wrist_tracking_error,'roll':h.forearm_roll,'force':h.applied_force,'error':h.position_error,'charge':h.throw_charge,'stamina':h.stamina});yield
g=cases()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);h.release()
 for a,(t,mesh,mass,sim) in snapshots.items():
  c=a.static_mesh_component;c.set_simulate_physics(False);c.set_static_mesh(mesh);a.set_actor_transform(t,False,True);c.set_mass_override_in_kg('None',mass,True);c.set_enable_gravity(True);c.set_simulate_physics(sim)
 p.set_actor_location(old_location,False,True);pc.set_control_rotation(old_rotation);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 Path(unreal.Paths.project_saved_dir(),'heavy_charge_diagnostic.json').write_text(json.dumps({'error':error,'samples':rows},indent=2));unreal.log('HEAVY_CHARGE_DONE '+error)
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
