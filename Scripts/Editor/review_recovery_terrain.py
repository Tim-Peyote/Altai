"""PIE-only terrain integration: slopes, uneven supports, ceiling, disappearing floor."""
import unreal,time,json,traceback,math,sys
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);body=pc.body_dynamics
unreal.AltaiEditorLibrary.set_physics_test_mode(True);pc.hands.release();p.un_crouch();p.set_first_person(False)
original=p.get_actor_transform();fixtures=[];rows=[];samples=[];case=''
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
actors={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor)}
pool=[actors[n] for n in ['Throw_Target_1kg','Throw_Bottle_1kg','Throw_Target_3kg']];saved={}
def block(location,scale,rotation=unreal.Rotator()):
 a=pool.pop(0);c=a.static_mesh_component
 saved[a]=(a.get_actor_transform(),c.static_mesh,c.is_simulating_physics(),c.get_collision_profile_name())
 c.set_simulate_physics(False);c.set_static_mesh(cube);c.set_collision_profile_name('BlockAll')
 a.set_actor_location(location,False,True);a.set_actor_rotation(rotation,True);a.set_actor_scale3d(scale);fixtures.append(a);return a
def release_fixture(a):
 transform,mesh,sim,profile=saved.pop(a);c=a.static_mesh_component
 c.set_simulate_physics(False);c.set_static_mesh(mesh);a.set_actor_transform(transform,False,True);c.set_collision_profile_name(profile);c.set_simulate_physics(sim)
 fixtures.remove(a);pool.append(a)
floor=block(unreal.Vector(9000,-1800,3390),unreal.Vector(60,60,.2))
names=['pelvis','spine_01','spine_02','spine_03','spine_04','spine_05','neck_01','neck_02','head','clavicle_l','clavicle_r','thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r','upperarm_l','lowerarm_l','hand_l','upperarm_r','lowerarm_r','hand_r']
def wait(seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:yield
def recover(before,timeout=24):
 end=time.monotonic()+timeout
 while body.recoveries==before and time.monotonic()<end:yield
 if body.recoveries==before:raise RuntimeError(case+': '+body.hint)
def start():
 p.set_actor_location(unreal.Vector(9000,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True)
 p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING);p.character_movement.stop_movement_immediately()
 pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=75,roll=0));yield from wait(1.4)
 body.test_fall()
def cases():
 global case
 for angle in [10,20]:
  case='slope_'+str(angle);floor.set_actor_rotation(unreal.Rotator(pitch=angle,yaw=0,roll=0),True)
  before=body.recoveries;yield from start();yield from recover(before)
  rows.append({'case':case,'passed':not pc.is_move_input_ignored(),'recovered':True})
 floor.set_actor_rotation(unreal.Rotator(),True)
 case='uneven';rocks=[block(unreal.Vector(8950,-1800,3404),unreal.Vector(.7,3,.08)),block(unreal.Vector(9050,-1800,3407),unreal.Vector(.7,3,.14))]
 rocks[1].static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere'))
 before=body.recoveries;yield from start();yield from recover(before)
 rows.append({'case':case,'passed':True,'recovered':True,'curved_support':True})
 for rock in rocks:release_fixture(rock)
 case='ceiling';before=body.recoveries;yield from start();yield from wait(.5)
 ceiling=block(unreal.Vector(9000,-1800,3520),unreal.Vector(8,8,.2));yield from wait(4)
 blocked=body.recoveries==before and body.state!=unreal.AltaiBodyState.GETTING_UP and body.recovery_blocked
 release_fixture(ceiling);yield from recover(before)
 rows.append({'case':case,'passed':blocked,'waited_for_clearance':blocked,'recovered':True})
 case='lost_support';before=body.recoveries;yield from start();end=time.monotonic()+15
 while body.state!=unreal.AltaiBodyState.GETTING_UP and time.monotonic()<end:yield
 if body.state!=unreal.AltaiBodyState.GETTING_UP:raise RuntimeError('No recovery to interrupt')
 yield from wait(1.2);falls=body.falls;floor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
 end=time.monotonic()+1
 while body.falls==falls and time.monotonic()<end:yield
 resumed=body.falls>falls and body.state==unreal.AltaiBodyState.FALLING
 floor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
 yield from recover(before)
 rows.append({'case':case,'passed':resumed,'returned_to_physics':resumed,'recovered':True})
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 if body.state==unreal.AltaiBodyState.BALANCED:
  p.set_actor_transform(original,False,True)
  for a in list(fixtures):release_fixture(a)
 sys.path.insert(0,str(Path(unreal.Paths.project_dir())/'Scripts/MCP'))
 from analyze_climbing_joints import sub,dot,cross,norm,rotate
 joints={}
 for side in ['l','r']:
  for a,b,c in [('thigh','calf','foot'),('upperarm','lowerarm','hand')]:
   a,b,c=[n+'_'+side for n in [a,b,c]];values=[]
   for r in samples:
    u=norm(sub(r['bones'][b],r['bones'][a]));v=norm(sub(r['bones'][c],r['bones'][b]));axis=rotate(r['rotations'][a],[0,0,1]);values.append(math.degrees(math.atan2(dot(cross(u,v),axis),dot(u,v))))
   joints[b]={'min':min(values,default=-999),'max':max(values,default=999)}
 safe=all(j['min']>=0 and j['max']<=145.5 for j in joints.values())
 Path(unreal.Paths.project_saved_dir(),'recovery_terrain_review.json').write_text(json.dumps({'passed':not error and len(rows)==5 and all(r['passed'] for r in rows) and safe,'cases':rows,'joints':joints,'error':error,'samples':samples},indent=2))
g=cases()
def tick(dt):
 try:
  next(g)
  if body.state==unreal.AltaiBodyState.GETTING_UP:
   contacts=[{'goal':list(c.goal.to_tuple()),'normal':list(c.normal.to_tuple()),'weight':c.weight} for c in body.recovery_contacts]
   samples.append({'case':case,'state':str(body.state),'progress':body.recovery_progress,'face_up':body.face_up,'contacts':contacts,'bones':{n:list(p.mesh.get_socket_location(n).to_tuple()) for n in names},'rotations':{n:list(p.mesh.get_socket_rotation(n).quaternion().to_tuple()) for n in names}})
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
