"""PIE integration checks for surfaces, precipitation, rejection, cancel and jump."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context,AltaiLabTools
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.wall_climbing;t=pc.traversal;h=pc.hands;weather=pc.weather
unreal.AltaiEditorLibrary.set_physics_test_mode(True);p.custom_time_dilation=1;p.un_crouch();h.release();t.cancel_climb();wall.release_wall();wall.input_from_player=False
rows=[];result={};snapshot=None

def wait(seconds,intent=None):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if intent:wall.set_move_intent(unreal.Vector2D(*intent))
  yield

def top_setup():
 wall.release_wall();t.cancel_climb();p.un_crouch();p.character_movement.stop_movement_immediately()
 rock=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Climbing_Test_Rough');c,e=rock.get_actor_bounds(False)
 p.set_actor_location(unreal.Vector(c.x+e.x+44,c.y,c.z+e.z-55),False,True);p.set_actor_rotation(unreal.Rotator(yaw=180),True);pc.set_control_rotation(unreal.Rotator(yaw=180));assert wall.attach_wall();return c,e

def run():
 global snapshot
 AltaiLabTools.set_lab_weather(1,14)
 for kind in ['RoughRock','SmoothRock','MossyRock']:
  AltaiLabTools.prepare_wall_height(kind,100);yield from wait(1);z=p.get_actor_location().z
  row={'surface':kind,'height_cm':100,'dry_grip':wall.grip};rows.append(row);yield from wait(4,(0,1));row['up_cm']=p.get_actor_location().z-z;wall.set_move_intent(unreal.Vector2D());yield from wait(.7);z=p.get_actor_location().z;yield from wait(4,(0,-1));row['down_cm']=z-p.get_actor_location().z;row['held']=wall.attached
 wall.release_wall();rain=max(range(len(weather.presets)),key=lambda i:weather.presets[i].rain);weather.select_preset(rain,True);yield from wait(15)
 for kind in ['RoughRock','SmoothRock','MossyRock']:
  AltaiLabTools.prepare_wall_height(kind,100);yield from wait(1);row=next(r for r in rows if r['surface']==kind);row['wet_grip']=wall.grip;row['wetness']=wall.wetness;yield from wait(.5)
 slips=wall.slips;yield from wait(5);result['wet_moss_slips']=wall.slips>slips;result['wet_moss_falls']=not wall.attached
 wall.release_wall();AltaiLabTools.set_lab_weather(1,14)
 # A real blocking rigid-body fixture must reject the mantle before releasing wall support.
 c,e=top_setup();yield from wait(1)
 block=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Throw_Target_3kg');comp=block.static_mesh_component
 snapshot=(block,block.get_actor_transform(),comp.static_mesh,comp.is_simulating_physics(),comp.get_mass())
 comp.set_simulate_physics(False);comp.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));comp.set_collision_profile_name('BlockAll');block.set_actor_scale3d(unreal.Vector(2,2,3));block.set_actor_location(unreal.Vector(c.x,c.y,c.z+e.z+100),False,True)
 result['blocked_rejected']=not t.try_climb_from_wall() and wall.attached and not t.climbing
 restore();result['blocked_hint']=t.hint
 # Abort during the crouched rock-over must release its input lock and movement state.
 result['cancel_started']=t.try_climb_from_wall();yield from wait(1.35);t.cancel_climb();yield from wait(.15)
 result['cancel_released']=not t.climbing and not pc.is_move_input_ignored() and not p.actor_has_tag('AltaiMantling')
 p.un_crouch();yield from wait(.5);AltaiLabTools.prepare_wall_height('RoughRock',300);yield from wait(1)
 result['jump']=wall.jump_off();yield from wait(.15);result['jump_released']=not wall.attached and not pc.is_move_input_ignored();result['jump_speed']=p.get_velocity().length()
 # Missing edge cannot enter the reverse transition.
 p.set_actor_location(unreal.Vector(4585,-3280,450),False,True);yield from wait(1);result['no_edge_rejected']=not t.try_descend()

def restore():
 global snapshot
 if snapshot:
  a,trans,mesh,sim,mass=snapshot;c=a.static_mesh_component;c.set_simulate_physics(False);c.set_static_mesh(mesh);a.set_actor_transform(trans,False,True);c.set_collision_profile_name('PhysicsActor');c.set_mass_override_in_kg('None',mass,True);c.set_simulate_physics(sim);snapshot=None

g=run()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);restore();wall.set_move_intent(unreal.Vector2D());p.custom_time_dilation=1;unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 checks=[all(r.get('up_cm',0)>20 and r.get('down_cm',0)>20 and r.get('held') for r in rows),all(r.get('wet_grip',2)<r['dry_grip']-.08 for r in rows),result.get('wet_moss_slips'),result.get('wet_moss_falls'),result.get('blocked_rejected'),result.get('cancel_started'),result.get('cancel_released'),result.get('jump_released'),result.get('jump_speed',0)>100,result.get('no_edge_rejected')]
 Path(unreal.Paths.project_saved_dir(),'climbing_boundaries_review.json').write_text(json.dumps({'passed':all(checks) and not error,'error':error,'checks':checks,'surfaces':rows,'results':result},indent=2));unreal.log('CLIMBING_BOUNDARIES_DONE '+error)
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
