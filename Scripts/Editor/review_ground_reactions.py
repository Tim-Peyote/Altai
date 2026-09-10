"""PIE regression: braking, frontal/oblique wall impact, recovery next to wall."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);b=pc.body_dynamics;m=p.character_movement
old=p.get_actor_transform();pc.hands.release();p.set_first_person(False)
actors={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor)};fixtures=[]
def block(name,loc,scale):
 a=actors[name];c=a.static_mesh_component;fixtures.append((a,a.get_actor_transform(),c.static_mesh,c.is_simulating_physics(),c.get_collision_profile_name()))
 c.set_simulate_physics(False);c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));c.set_collision_profile_name('BlockAll');a.set_actor_scale3d(unreal.Vector(*scale));a.set_actor_location(unreal.Vector(*loc),False,True);a.set_actor_rotation(unreal.Rotator(),True);return a
floor=block('Throw_Target_1kg',(9000,-1800,3390),(60,60,.2));wall=block('Throw_Target_3kg',(9300,-1800,3600),(.4,25,4))
rows=[];samples=[];case='';motion=[]
def wait(t,direction=None):
 end=time.monotonic()+t
 while time.monotonic()<end:
  if direction:p.add_movement_input(unreal.Vector(*direction),1,False)
  yield

def reset(x=9000,y=-1800):
 p.set_actor_location(unreal.Vector(x,y,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True);m.set_movement_mode(unreal.MovementMode.MOVE_WALKING);m.stop_movement_immediately();yield from wait(1.5)
def run():
 global case
 case='braking';yield from reset(8000);yield from wait(1.5,(1,0,0));v=m.velocity.length();start=p.get_actor_location();begin=time.monotonic()
 while m.velocity.length()>5 and time.monotonic()-begin<3:yield
 rows.append({'case':case,'speed':v,'seconds':time.monotonic()-begin,'distance':(p.get_actor_location()-start).length(),'passed':m.velocity.length()<5 and 45<(p.get_actor_location()-start).length()<180})
 case='run_braking';m.velocity=unreal.Vector(725,0,0);start=p.get_actor_location();begin=time.monotonic()
 while m.velocity.length()>5 and time.monotonic()-begin<3:yield
 rows.append({'case':case,'speed':725,'seconds':time.monotonic()-begin,'distance':(p.get_actor_location()-start).length(),'passed':m.velocity.length()<5 and 90<(p.get_actor_location()-start).length()<300})
 for case,direction in [('wall_front',(1,0,0)),('wall_oblique',(1,.4,0))]:
  yield from reset();falls=b.falls;peak=0;end=time.monotonic()+2
  while time.monotonic()<end:
   p.add_movement_input(unreal.Vector(*direction),1,False);peak=max(peak,b.reaction);yield
  rows.append({'case':case,'peak_reaction':peak,'falls':b.falls-falls,'position':list(p.get_actor_location().to_tuple()),'passed':b.falls==falls and p.get_actor_location().x<9240 and peak>.1})
 for i,(x,y,dx,dy) in enumerate([(9190,-1800,1,0),(9200,-1700,1,.5),(9200,-1900,1,-.5)]):
  case='wall_recovery_'+str(i);yield from reset(x,y);before=b.recoveries
  b.apply_body_impulse(unreal.Vector(dx*35000,dy*35000,0),p.mesh.get_socket_location('spine_03')+unreal.Vector(0,0,15),'spine_03');end=time.monotonic()+22
  while b.recoveries==before and time.monotonic()<end:yield
  ok=b.recoveries>before and not pc.is_move_input_ignored();rows.append({'case':case,'passed':ok,'state':str(b.state),'issue':b.recovery_terrain_issue,'hint':b.hint})
  if not ok:raise RuntimeError(case+' stuck: '+b.hint)
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle)
 if b.state==unreal.AltaiBodyState.BALANCED:
  p.set_actor_transform(old,False,True)
  for a,t,mesh,sim,profile in fixtures:
   c=a.static_mesh_component;c.set_simulate_physics(False);c.set_static_mesh(mesh);a.set_actor_transform(t,False,True);c.set_collision_profile_name(profile);c.set_simulate_physics(sim)
 Path(unreal.Paths.project_saved_dir(),'ground_reactions_review.json').write_text(json.dumps({'passed':not error and len(rows)==7 and all(r['passed'] for r in rows),'cases':rows,'error':error,'samples':samples},indent=2))
g=run()
def tick(dt):
 try:
  next(g);samples.append({'case':case,'state':str(b.state),'issue':b.recovery_terrain_issue,'progress':b.recovery_progress,'hip':list(p.mesh.get_socket_location('pelvis').to_tuple()),'hip_speed':p.mesh.get_physics_linear_velocity('pelvis').length(),'spin':b.angular_velocity.length(),'speed':m.velocity.length(),'input_locked':pc.is_move_input_ignored()})
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
