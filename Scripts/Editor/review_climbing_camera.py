"""First-person lower-to-hang camera and view-mode restoration integration review."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.wall_climbing;t=pc.traversal;h=pc.hands
unreal.AltaiEditorLibrary.set_physics_test_mode(True);p.custom_time_dilation=1;t.cancel_climb();wall.release_wall();h.release();p.un_crouch();wall.input_from_player=False
rock=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Climbing_Test_Rough');center,ext=rock.get_actor_bounds(False);top=center.z+ext.z
p.set_actor_location(unreal.Vector(center.x,center.y,top+98),False,True);p.character_movement.stop_movement_immediately();p.set_actor_rotation(unreal.Rotator(yaw=0),True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));p.set_first_person(True)
rows=[];result={};phase='prepare'
def wait(s):
 end=time.monotonic()+s
 while time.monotonic()<end:yield
def run():
 global phase
 yield from wait(1.5);phase='lower';result['started']=t.try_descend();yield from wait(3.8);result['hanging']=wall.attached;result['yaw']=pc.get_control_rotation().yaw;result['pitch']=pc.get_control_rotation().pitch
 phase='raise';result['raise']=t.try_climb_from_wall();yield from wait(.7);p.set_first_person(False);yield from wait(.25);p.set_first_person(True);yield from wait(3)
 result['on_top']=p.character_movement.is_moving_on_ground();result['look_restored']=not pc.is_look_input_ignored();result['move_restored']=not pc.is_move_input_ignored();result['fp_yaw_restored']=p.use_controller_rotation_yaw

g=run()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 checks=[result.get('started'),result.get('hanging'),abs(abs(result.get('yaw',0))-180)<2,abs(result.get('pitch',0)+20)<2,result.get('raise'),result.get('on_top'),result.get('move_restored'),result.get('fp_yaw_restored')]
 Path(unreal.Paths.project_saved_dir(),'climbing_camera_review.json').write_text(json.dumps({'passed':all(checks) and not error,'checks':checks,'error':error,'results':result,'samples':rows},indent=2));unreal.log('CLIMBING_CAMERA_DONE '+error)
def tick(dt):
 try:
  next(g);rows.append({'time':time.monotonic(),'phase':phase,'progress':t.progress,'camera':list(pc.player_camera_manager.get_camera_location().to_tuple()),'yaw':pc.get_control_rotation().yaw,'root':list(p.get_actor_location().to_tuple()),'first_person':p.first_person})
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
