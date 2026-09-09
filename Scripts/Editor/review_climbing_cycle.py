"""PIE review: sequential support, ascent/descent, unilateral hang, top-out and reverse."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context,AltaiLabTools
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.wall_climbing;t=pc.traversal;h=pc.hands
unreal.AltaiEditorLibrary.set_physics_test_mode(True);p.un_crouch();h.release();t.cancel_climb();wall.release_wall();wall.input_from_player=False
rows=[];results={};stage='prepare';began=time.monotonic()
bones=['hand_l','hand_r','foot_l','foot_r']
def pause(seconds,intent=None):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if intent:wall.set_move_intent(unreal.Vector2D(*intent))
  yield
def select(i):
 while wall.selected_limb!=i:wall.select_next_limb()
def run():
 global stage
 AltaiLabTools.set_lab_weather(1,14);AltaiLabTools.prepare_wall_height('RoughRock',250);p.set_first_person(False);pc.set_control_rotation(unreal.Rotator(pitch=-5,yaw=105,roll=0));
 results['initial_sequential']=wall.moving_limb==0 and not wall.contact_active[0] and wall.contact_active[1]
 yield from pause(1.2);z=p.get_actor_location().z;stage='ascent';yield from pause(10,(0,1));results['ascent_gain']=p.get_actor_location().z-z
 wall.set_move_intent(unreal.Vector2D());yield from pause(1);z=p.get_actor_location().z;stage='descent';yield from pause(10,(0,-1));results['descent_gain']=z-p.get_actor_location().z;wall.set_move_intent(unreal.Vector2D());yield from pause(1)
 stage='one_hand';p.set_first_person(True);yield from pause(.8);wall.assisted_stepping=False;select(0);wall.release_selected_contact();select(3);wall.release_selected_contact();yield from pause(1.5);results['one_hand_one_foot']=wall.attached and list(wall.contact_active)==[False,True,True,False]
 # Restore via normal player aim and placement, one limb at a time.
 for i in [0,3]:
  select(i);target=p.get_actor_location()+p.get_actor_forward_vector()*44+p.get_actor_right_vector()*((1 if i%2 else -1)*(28 if i<2 else 22))+unreal.Vector(0,0,45 if i<2 else -60)
  pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),target));yield from pause(.15);results['regrip_'+str(i)]=wall.place_contact();yield from pause(1)
 wall.assisted_stepping=True;stage='top_setup';wall.release_wall()
 rock=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Climbing_Test_Rough');center,ext=rock.get_actor_bounds(False);top=center.z+ext.z
 p.set_actor_location(unreal.Vector(center.x+ext.x+44,center.y,top-55),False,True);p.set_actor_rotation(unreal.Rotator(yaw=180),True);pc.set_control_rotation(unreal.Rotator(yaw=180));results['top_attach']=wall.attach_wall();yield from pause(1)
 stage='mantle';results['mantle_start']=t.try_climb_from_wall();results['mantle_hint']=t.hint;count=t.completed_climbs;yield from pause(3.6);results['mantle_completed']=t.completed_climbs>count;results['grounded']=p.character_movement.is_moving_on_ground();results['top_height_error']=p.get_actor_location().z-p.capsule_component.get_scaled_capsule_half_height()-top
 stage='lower';pc.set_control_rotation(unreal.Rotator(yaw=0));count=t.completed_descents;results['lower_start']=t.try_descend();results['lower_hint']=t.hint;yield from pause(3.7);results['lower_completed']=t.completed_descents>count and wall.attached
 stage='down_after_lower';z=p.get_actor_location().z;yield from pause(4,(0,-1));results['lower_then_down']=z-p.get_actor_location().z
 wall.set_move_intent(unreal.Vector2D());stage='done'
g=run()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);wall.set_move_intent(unreal.Vector2D());unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 import sys
 sys.path.insert(0,str(Path(unreal.Paths.project_dir())/'Scripts/MCP'))
 from analyze_climbing_joints import analyze
 joint_review=analyze(rows)
 checks=[joint_review['passed'],results.get('initial_sequential'),results.get('ascent_gain',0)>60,results.get('descent_gain',0)>60,results.get('one_hand_one_foot'),results.get('regrip_0'),results.get('regrip_3'),results.get('mantle_completed'),results.get('grounded'),abs(results.get('top_height_error',999))<6,results.get('lower_completed'),results.get('lower_then_down',0)>20]
 Path(unreal.Paths.project_saved_dir(),'climbing_cycle_review.json').write_text(json.dumps({'passed':all(checks) and not error,'checks':checks,'joint_review':joint_review,'error':error,'results':results,'samples':rows},indent=2));unreal.log('CLIMBING_CYCLE_DONE '+error)
def tick(dt):
 try:
  next(g)
  # Fixture setup teleports the actor; allow the skeletal component to evaluate at the new location.
  # A separate entry test covers attachment without any teleport in the sampled interval.
  if stage=='prepare' and time.monotonic()-began<.12:return
  rows.append({'t':time.monotonic()-began,'stage':stage,'phase':stage,'bones':{b:list(p.mesh.get_socket_location(b).to_tuple()) for b in ['thigh_l','thigh_r','calf_l','calf_r','foot_l','foot_r','upperarm_l','upperarm_r','lowerarm_l','lowerarm_r','hand_l','hand_r']},'rotations':{b:list(p.mesh.get_socket_rotation(b).quaternion().to_tuple()) for b in ['thigh_l','thigh_r','calf_l','calf_r','upperarm_l','upperarm_r','lowerarm_l','lowerarm_r']},'z':p.get_actor_location().z,'active':list(wall.contact_active),'moving':wall.moving_limb,'attached':wall.attached,'stamina':wall.stamina,'progress':t.progress,'mantling':t.climbing,'camera':list(pc.player_camera_manager.get_camera_location().to_tuple()),'weights':list(h.contact_weights),'errors':[(p.mesh.get_socket_location(b)-h.contact_goals[i]).length() for i,b in enumerate(bones)],'hint':t.hint if t.climbing else wall.hint})
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
