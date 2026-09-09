"""PIE integration review of real rigid bodies, reachable contacts and throw modes."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.hands
pc.traversal.cancel_climb();pc.wall_climbing.release_wall();h.release();p.un_crouch();p.set_first_person(True);unreal.AltaiEditorLibrary.set_physics_test_mode(True)
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
 for mass,geo,mode in [(.25,'SM_HandFlask','hold'),(1,'SM_HandBottle','rotate'),(5,'SM_FieldStone','hold'),(12,'SM_FieldBucket','hold'),(20,'SM_FieldStone','hold'),(1,'SM_HandBottle','quick'),(1,'SM_HandBottle','full'),(20,'SM_FieldStone','full'),(.25,'SM_HandFlask','swing')]:
  h.release();p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(6300,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True);pc.set_control_rotation(unreal.Rotator())
  c=spawn('/Game/Altai/Environment/Geometry/'+geo,(6360,-1800,3530),mass)
  c.set_enable_gravity(False)
  yield from pause(.4)
  pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),c.get_center_of_mass()))
  yield from pause(.2)
  row={'mass':mass,'mode':mode,'grab':h.try_grab(False),'prop_simulating':c.is_simulating_physics(),'prop_position':str(c.get_center_of_mass()),'camera':str(pc.player_camera_manager.get_camera_location())};rows.append(row);c.set_enable_gravity(True)
  yield from pause(1.6)
  row.update(held=h.held==c,two_hands=h.two_hands,position_error=h.position_error,speed_scale=h.carry_speed_scale,hint=h.hint)
  if h.held!=c:c.set_simulate_physics(False);continue
  row['wrist_error']=(p.mesh.get_socket_location('hand_r')-h.contact_goals[1]).length();row['reach']=(p.mesh.get_socket_location('upperarm_r')-h.contact_goals[1]).length()
  if mode=='hold':
   row['one_hand_allowed']=h.set_two_hand_grip(False);h.set_two_hand_grip(True);yield from pause(1);row['two_hand_wrist_error']=(p.mesh.get_socket_location('hand_l')-h.contact_goals[0]).length()
  if mode=='rotate':
   h.set_held_rotation_mode(True);row['rotation_locked_view']=pc.is_look_input_ignored();row['rotation_axes']=[]
   for axis in range(3):
    before=c.get_owner().get_actor_rotation();v=[0,0,0];v[axis]=15
    for i in range(6):h.rotate_held(unreal.Vector(*v));yield from pause(.08)
    yield from pause(1.2)
    row['rotation_axes'].append({'axis':axis,'before':str(before),'after':str(c.get_owner().get_actor_rotation()),'wrist_error':(p.mesh.get_socket_location('hand_r')-h.contact_goals[1]).length(),'held':h.held==c})
   h.set_held_rotation_mode(False);row['rotation_view_restored']=not pc.is_look_input_ignored();yield from pause(.5)
   row['charge_start']=h.begin_charge_throw();pc.toggle_developer_panel();row['panel_cancels_charge']=not h.charging_throw and h.held==c;pc.toggle_developer_panel();row['panel_restores_view']=not pc.is_look_input_ignored()
  if mode in ['quick','full']:
   pc.set_control_rotation(unreal.Rotator(pitch=15,yaw=0,roll=0));row['charge_start']=h.begin_charge_throw();yield from pause(.08 if mode=='quick' else 1.6);row['charge']=h.throw_charge;row['windup_wrist_error']=(p.mesh.get_socket_location('hand_r')-h.contact_goals[1]).length();row['release_requested']=h.release_charged_throw();yield from pause(.35);row.update(released=h.held is None,throw_speed=h.last_throw_speed,throw_energy=h.last_throw_energy,actual_speed=c.get_physics_linear_velocity().length())
  if mode=='swing':
   row['motion_start']=h.begin_hand_motion();h.move_held_hand(unreal.Vector2D(0,-65));yield from pause(.4);h.move_held_hand(unreal.Vector2D(0,120));yield from pause(.12);before=c.get_physics_linear_velocity();h.release();after=c.get_physics_linear_velocity();row.update(swing_speed=before.length(),release_velocity_difference=(before-after).length(),look_restored=not pc.is_look_input_ignored())
  h.release();c.set_simulate_physics(False);yield from pause(.2)

g=cases()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);h.release()
 for a,(t,mesh,mass,sim) in snapshots.items():
  c=a.static_mesh_component;c.set_simulate_physics(False);c.set_static_mesh(mesh);a.set_actor_transform(t,False,True);c.set_mass_override_in_kg('None',mass,True);c.set_enable_gravity(True);c.set_simulate_physics(sim)
 p.set_actor_location(old_location,False,True);pc.set_control_rotation(old_rotation);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 checks=[not error,len(rows)==9,all(r.get('grab') and r.get('held') and r.get('wrist_error',999)<2 for r in rows),all(r.get('one_hand_allowed')==(r['mass']<=6) for r in rows if r['mode']=='hold'),all(r.get('two_hand_wrist_error',999)<2 for r in rows if r['mode']=='hold')]
 if len(rows)==9:checks.extend([all(a['held'] and a['wrist_error']<2 for a in rows[1].get('rotation_axes',[])),rows[1].get('panel_cancels_charge',False),rows[1].get('panel_restores_view',False),all(r.get('released') and r.get('windup_wrist_error',999)<2 for r in rows if r['mode'] in ['quick','full']),rows[6].get('throw_speed',0)>rows[5].get('throw_speed',99999)*2,rows[7].get('throw_speed',99999)<rows[6].get('throw_speed',0),rows[8].get('release_velocity_difference',999)<.001,rows[8].get('swing_speed',0)>50,rows[8].get('look_restored',False)])
 Path(unreal.Paths.project_saved_dir(),'object_manipulation_review.json').write_text(json.dumps({'passed':all(checks),'checks':checks,'error':error,'cases':rows},indent=2));unreal.log('ALTAI_OBJECT_REVIEW_DONE '+error)
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
