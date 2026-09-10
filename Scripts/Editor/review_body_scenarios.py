"""Deterministic PIE cases: light trip, high fall, impulses, mass, input ownership.
All fixtures are restored; the map and input settings are not saved.
"""
import unreal,time,json,traceback,math,sys
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);b=pc.body_dynamics;h=pc.hands
unreal.AltaiEditorLibrary.set_physics_test_mode(True);pc.traversal.cancel_climb();pc.wall_climbing.release_wall();h.release();p.un_crouch();p.set_first_person(False)
old=p.get_actor_transform();oldmass=p.character_movement.mass;oldview=pc.get_control_rotation()
a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Throw_Target_1kg');c=a.static_mesh_component
oldfixture=(a.get_actor_transform(),c.static_mesh,c.is_simulating_physics(),c.get_collision_profile_name());c.set_simulate_physics(False);c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));c.set_collision_profile_name('BlockAll');a.set_actor_scale3d(unreal.Vector(60,60,.2));a.set_actor_location(unreal.Vector(9000,-1800,3390),False,True)
rows=[];samples=[];case='';g=None
names=['spine_01','spine_02','spine_03','spine_04','spine_05','neck_01','neck_02','head','clavicle_l','clavicle_r','pelvis','thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r','upperarm_l','lowerarm_l','hand_l','upperarm_r','lowerarm_r','hand_r']
def wait(seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:yield

def reset(mass=100):
 p.character_movement.mass=mass;p.set_actor_location(unreal.Vector(9000,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True);pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=75,roll=0));p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING);p.character_movement.stop_movement_immediately();yield from wait(1.4)
def cases():
 global case
 yield from reset();case='light';before=b.falls;b.test_stumble();peak=0
 end=time.monotonic()+1.4
 while time.monotonic()<end:peak=max(peak,b.reaction);yield
 rows.append({'case':case,'passed':peak>.4 and b.falls==before and not pc.is_move_input_ignored(),'peak':peak})
 for case,mass,kind in [('height',100,'height'),('side_light',60,'side'),('side_heavy',140,'side'),('back',100,'back')]:
  yield from reset(mass);before=b.falls;recoveries=b.recoveries
  impulse_received=True
  if kind=='height':p.set_actor_location(unreal.Vector(9000,-1800,4296),False,True);p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FALLING);p.character_movement.velocity=unreal.Vector(160,100,-300)
  else:
   impulse=unreal.Vector(0,35000,0) if kind=='side' else unreal.Vector(-35000,0,0)
   before_impulse=p.mesh.get_physics_linear_velocity('spine_02')
   b.apply_body_impulse(impulse,p.mesh.get_socket_location('spine_03')+unreal.Vector(0,12,20),'spine_03')
   yield from wait(.08)
   delta=p.mesh.get_physics_linear_velocity('spine_02')-before_impulse
   impulse_received=delta.dot(impulse.normal())>100
  peakspin=0;seen=False;blocked=False;firstperson=False;end=time.monotonic()+18
  while time.monotonic()<end and b.recoveries==recoveries:
   seen=seen or b.falls>before;peakspin=max(peakspin,b.angular_velocity.length())
   if seen and not blocked:blocked=not h.try_grab(False) and not pc.wall_climbing.attach_wall() and not pc.traversal.try_climb()
   if seen and not firstperson:p.set_first_person(True);firstperson=True
   samples.append({'case':case,'state':str(b.state),'progress':b.recovery_progress,'face_up':b.face_up,'acquire_duration':b.recovery_acquire,'bones':{n:list(p.mesh.get_socket_location(n).to_tuple()) for n in names},'rotations':{n:list(p.mesh.get_socket_rotation(n).quaternion().to_tuple()) for n in names},'camera':list(pc.player_camera_manager.get_camera_location().to_tuple())})
   yield
  ok=impulse_received and seen and b.recoveries>recoveries and not pc.is_move_input_ignored() and blocked and abs(b.physical_mass-mass)<.1
  rows.append({'case':case,'passed':ok,'impulse_received':impulse_received,'fell':seen,'recovered':b.recoveries>recoveries,'input_restored':not pc.is_move_input_ignored(),'interactions_blocked':blocked,'mass':b.physical_mass,'peak_spin':peakspin})
  p.set_first_person(False)
  if not ok:raise RuntimeError('Body scenario did not recover: '+case)

def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 # Restore fixture only when the body is no longer resting on it.
 if b.state==unreal.AltaiBodyState.BALANCED:
  p.set_actor_transform(old,False,True);p.character_movement.mass=oldmass;pc.set_control_rotation(oldview)
  c.set_simulate_physics(False);c.set_static_mesh(oldfixture[1]);a.set_actor_transform(oldfixture[0],False,True);c.set_collision_profile_name(oldfixture[3]);c.set_simulate_physics(oldfixture[2])
 sys.path.insert(0,str(Path(unreal.Paths.project_dir())/'Scripts/MCP'))
 from analyze_climbing_joints import sub,dot,cross,norm,rotate
 joints={}
 for side in ['l','r']:
  for upper,joint,end in [('thigh','calf','foot'),('upperarm','lowerarm','hand')]:
   upper,joint,end=[n+'_'+side for n in [upper,joint,end]];angles=[]
   for r in samples:
    u=norm(sub(r['bones'][joint],r['bones'][upper]));v=norm(sub(r['bones'][end],r['bones'][joint]));axis=rotate(r['rotations'][upper],[0,0,1]);angles.append(math.degrees(math.atan2(dot(cross(u,v),axis),dot(u,v))))
   joints[joint]={'min':min(angles,default=-999),'max':max(angles,default=999)}
 safe=all(v['min']>=0 and v['max']<=145.5 for v in joints.values())
 Path(unreal.Paths.project_saved_dir(),'body_scenarios_review.json').write_text(json.dumps({'passed':not error and len(rows)==5 and all(r['passed'] for r in rows) and safe,'joint_limits_passed':safe,'joints':joints,'cases':rows,'error':error,'samples':samples},indent=2))
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
g=cases();handle=unreal.register_slate_post_tick_callback(tick)
