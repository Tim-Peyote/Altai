"""Climb 10m continuously with real sequential regrips/rests; no upward teleports."""
import unreal,json,time,traceback
from pathlib import Path
from altai_lab_tools import context,AltaiLabTools
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.get_editor_property('wall_climbing');AltaiLabTools.set_lab_weather(1,14);AltaiLabTools.prepare_wall_height('RoughRock',0)
unreal.AltaiEditorLibrary.set_physics_test_mode(True);p.set_first_person(True)
start=p.get_actor_location().z;began=time.monotonic();samples=[];next_rest=200;rest_until=0;limb=None;rests=0;last_sample=0

def finish(passed,error=''):
 unreal.unregister_slate_post_tick_callback(handle);unreal.AltaiEditorLibrary.set_physics_test_mode(False);wall.set_move_intent(unreal.Vector2D(0,0))
 r={'passed':passed,'gain_cm':p.get_actor_location().z-start,'seconds':time.monotonic()-began,'rests':rests,'error':error,'samples':samples};Path(unreal.Paths.project_saved_dir(),'continuous_climb_validation.json').write_text(json.dumps(r,indent=2));unreal.log('CONTINUOUS_CLIMB_VALIDATION_'+('PASSED' if passed else 'FAILED'))
def tick(dt):
 global rest_until,limb,next_rest,rests,last_sample
 try:
  now=time.monotonic();gain=p.get_actor_location().z-start
  if now-last_sample>.5:samples.append({'gain':gain,'stamina':wall.get_editor_property('stamina'),'active':list(wall.get_editor_property('contact_active')),'moving':wall.get_editor_property('moving_limb')});last_sample=now
  if not wall.get_editor_property('attached'):finish(False,'Lost wall');return
  if gain>=1000:finish(True);return
  if now-began>220:finish(False,'Timeout');return
  if now<rest_until:wall.set_move_intent(unreal.Vector2D(0,0));return
  if gain>=next_rest or limb is not None:
   wall.set_move_intent(unreal.Vector2D(0,0))
   if wall.get_editor_property('moving_limb')!=-1:return
   if limb is None:limb=0
   if limb==4:limb=None;next_rest+=200;rest_until=now+16;rests+=1;return
   while wall.get_editor_property('selected_limb')!=limb:wall.select_next_limb()
   target=p.get_actor_location()+p.get_actor_forward_vector()*44+p.get_actor_right_vector()*((1 if limb%2 else -1)*(28 if limb<2 else 22))+unreal.Vector(0,0,45 if limb<2 else -60)
   pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),target))
   assert wall.place_contact(),'Rest contact outside reach';limb+=1;return
  wall.set_move_intent(unreal.Vector2D(0,1))
 except Exception:finish(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
