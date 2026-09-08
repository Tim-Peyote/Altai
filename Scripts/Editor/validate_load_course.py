"""Walk the authored beam with and without 20kg; no teleporting during either traversal."""
import unreal,time,json,sys,traceback
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_landform import height
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.get_editor_property('hands');pc.get_editor_property('wall_climbing').release_wall()
stone=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Loose_Stone_20kg')
report={};samples=[];case=0;phase='prepare';began=time.monotonic();on_beam=False;mass=0
base=height(5400,-4100);grip_local=[]
def stop(passed,error=''):
 errors=[max(s['live_wrists']) for s in samples if s.get('live_wrists')]
 if passed and (not errors or max(errors)>8):passed=False;error='Live wrist contact exceeded 8 cm: '+str(max(errors,default=0))
 unreal.unregister_slate_post_tick_callback(handle);p.character_movement.stop_movement_immediately();h.release()
 Path(unreal.Paths.project_saved_dir(),'load_course_validation.json').write_text(json.dumps({'passed':passed,'cases':report,'error':error,'samples':samples},indent=2));unreal.log('LOAD_COURSE_VALIDATION_'+('PASSED' if passed else 'FAILED'))
def tick(dt):
 global case,phase,began,on_beam,mass,grip_local
 try:
  now=time.monotonic()
  if mass and phase in ['hold','walk']:
   held=h.get_editor_property('held');samples.append({'phase':phase,'dt':dt,'live_wrists':[(p.mesh.get_socket_location(b)-unreal.MathLibrary.transform_location(held.get_world_transform(),grip_local[i])).length() for i,b in enumerate(['hand_l','hand_r'])] if held and grip_local and phase=='walk' else [],'carry_pose':h.get_editor_property('carry_pose'),'wrists':[(p.mesh.get_socket_location(b)-h.get_editor_property('contact_goals')[i]).length() for i,b in enumerate(['hand_l','hand_r'])] if held else [],'mass':h.get_editor_property('held_mass'),'error':h.get_editor_property('position_error'),'force':h.get_editor_property('applied_force'),'hint':h.get_editor_property('hint'),'pawn':str(p.get_actor_location()),'object':str(held.get_center_of_mass()) if held else None})
  if phase=='prepare':
   mass=0 if case==0 else 20;h.release();p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(5400,-3520,height(5400,-3520)+95),False,True);p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=270,roll=0),True);pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=270,roll=0))
   if mass:
    stone.static_mesh_component.set_physics_linear_velocity(unreal.Vector(0,0,0));stone.static_mesh_component.set_physics_angular_velocity_in_degrees(unreal.Vector(0,0,0));stone.set_actor_location(unreal.Vector(5500,-3520,height(5500,-3520)+15),False,True)
   began=now;phase='settle';on_beam=False;return
  if phase=='settle' and now-began>1:
   if mass:
    pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(p.get_actor_location()+unreal.Vector(0,0,55),stone.static_mesh_component.get_center_of_mass()))
    if not h.try_grab():stop(False,'Cannot pick course load');return
   began=now;phase='hold';return
  if phase=='hold' and now-began>3:
   if mass:grip_local=[unreal.MathLibrary.inverse_transform_location(h.get_editor_property('held').get_world_transform(),g) for g in h.get_editor_property('contact_goals')[:2]]
   pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=270,roll=0));p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=270,roll=0),True);phase='walk';began=now;return
  if phase=='walk':
   pos=p.get_actor_location()
   if abs(pos.y+4100)<100 and pos.z>base+160:on_beam=True
   if pos.y<-4650:
    report[str(mass)]={'on_beam':on_beam,'held':h.get_editor_property('held') is not None,'position':str(pos),'seconds':now-began,'max_speed':p.character_movement.max_walk_speed}
    if not on_beam or (mass and not h.get_editor_property('held')):stop(False,'Course contact or load lost');return
    if case==1:stop(True);return
    case+=1;phase='prepare';return
   if now-began>12:stop(False,'Course movement blocked at '+str(pos));return
   p.add_movement_input(unreal.Vector(0,-1,0),1,True)
 except Exception:stop(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
