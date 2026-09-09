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

ANATOMY_CASES=globals().get('ANATOMY_TEST_CASES',[(.25,'SM_HandFlask'),(1,'SM_HandBottle'),(.4,'SM_HandCup'),(.8,'SM_FieldStick'),(5,'SM_FieldStone'),(2,'SM_FieldBucket'),(12,'SM_FieldBucket'),(18,'SM_FieldCrate')])
def cases():
 for mass,geo in ANATOMY_CASES:
  h.release();p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(6300,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True);pc.set_control_rotation(unreal.Rotator())
  c=spawn('/Game/Altai/Environment/Geometry/'+geo,(6360,-1800,3530),mass);c.set_enable_gravity(False);yield from pause(.5)
  for k in range(10):pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),c.get_center_of_mass()));yield
  row={'mesh':geo,'mass':mass,'grab':h.try_grab(False)};rows.append(row);c.set_enable_gravity(True);yield from pause(1.8)
  row.update(held=h.held==c,hint=h.hint,wrist_angular_error=h.wrist_tracking_error,forearm_roll=h.forearm_roll,two_hands=h.two_hands)
  if h.held!=c:c.set_simulate_physics(False);continue
  row['wrist_position_error']=(p.mesh.get_socket_location('hand_r')-h.contact_goals[1]).length();row['axes']=[]
  local=unreal.MathLibrary.inverse_transform_location(c.get_owner().get_actor_transform(),h.contact_goals[1]);camera=pc.player_camera_manager.get_camera_location()
  for axis,sign in [(a,s) for a in range(3) for s in [1,-1]]:
   h.set_held_rotation_mode(True);v=[0,0,0];v[axis]=12*sign
   for k in range(60):h.rotate_held(unreal.Vector(*v))
   yield from pause(1)
   r={'axis':axis,'sign':sign,'held':h.held==c,'angles':list(h.grip_angles.to_tuple()),'limits':list(h.grip_angle_limits.to_tuple()),'effort':h.grip_rotation_effort,'wrist_angular_error':h.wrist_tracking_error,'forearm_roll':h.forearm_roll};row['axes'].append(r)
   r['contact_slip']=(unreal.MathLibrary.inverse_transform_location(c.get_owner().get_actor_transform(),h.contact_goals[1])-local).length()
   before=unreal.Vector(*h.grip_angles.to_tuple());h.set_held_rotation_mode(False);h.set_held_rotation_mode(True);r['reentry_reset']=(h.grip_angles-before).length();v[axis]=-2*sign;h.rotate_held(unreal.Vector(*v));r['reverse_response']=(h.grip_angles-before).length()
   # Return in hand-frame coordinates, with no dropping or invented regrip.
   for k in range(15):h.rotate_held(-h.grip_angles);yield
   yield from pause(.4)
  h.set_held_rotation_mode(False);row['camera_shift']=(pc.player_camera_manager.get_camera_location()-camera).length();row['view_restored']=not pc.is_look_input_ignored()
  h.release();c.set_simulate_physics(False);yield from pause(.3)
g=cases()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);h.release()
 for a,(t,mesh,mass,sim) in snapshots.items():
  c=a.static_mesh_component;c.set_simulate_physics(False);c.set_static_mesh(mesh);a.set_actor_transform(t,False,True);c.set_mass_override_in_kg('None',mass,True);c.set_enable_gravity(True);c.set_simulate_physics(sim)
 p.set_actor_location(old_location,False,True);pc.set_control_rotation(old_rotation);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 checks={'completed':not error and len(rows)==len(ANATOMY_CASES),'all_grabs':all(r.get('grab') and r.get('held') for r in rows),'neutral_wrist':all(r.get('wrist_angular_error',999)<5 and r.get('wrist_position_error',999)<2 for r in rows),'bounded_rotation':all(a['effort']<=1.001 and a['forearm_roll']<=75.01 and a['wrist_angular_error']<8 for r in rows for a in r.get('axes',[])),'fixed_contacts':all(a['contact_slip']<.01 for r in rows for a in r.get('axes',[])),'no_reentry_reset':all(a['reentry_reset']<.001 for r in rows for a in r.get('axes',[])),'reverse_responds':all(a['reverse_response']>.2 for r in rows for a in r.get('axes',[])),'held_at_limits':all(a['held'] for r in rows for a in r.get('axes',[])),'steady_view':all(r.get('camera_shift',999)<1 and r.get('view_restored') for r in rows)}
 Path(unreal.Paths.project_saved_dir(),globals().get('ANATOMY_REPORT_NAME','anatomical_grips_review.json')).write_text(json.dumps({'passed':all(checks.values()),'checks':checks,'error':error,'cases':rows},indent=2));unreal.log('ALTAI_ANATOMY_REVIEW_DONE '+error)
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
