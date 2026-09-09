"""Normal aim/grab/drag/release input path and live wrist contacts on each fixture."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.hands;h.release();p.set_first_person(True);p.un_crouch()
props=sorted(unreal.GameplayStatics.get_all_actors_of_class(w,unreal.AltaiArticulatedProp),key=lambda a:a.get_actor_label());report=[];idx=0;phase='prepare';began=time.monotonic();row={};samples=[]
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);h.release();p.un_crouch();p.character_movement.stop_movement_immediately()
 Path(unreal.Paths.project_saved_dir(),'furniture_standing_validation.json').write_text(json.dumps({'passed':not error and all(r.get('grab') and r.get('dragged',0)>3 and r.get('max_wrist_error',999)<2 and r.get('still_held') and r.get('look_restored') and r.get('tag_cleared') and r.get('carry_speed')==1 and r.get('max_camera_drag_shift',999)<1 for r in report),'error':error,'cases':report},indent=2));unreal.log('FURNITURE_HAND_TEST_DONE')
def tick(dt):
 global idx,phase,began,row,samples
 try:
  now=time.monotonic();a=props[idx]
  if phase in ['hold','drag']:
   camera=pc.player_camera_manager.get_camera_location();previous=row.get('last_camera',row.get('camera_before'));row['max_frame_camera_step']=max(row.get('max_frame_camera_step',0),(camera-unreal.Vector(*previous)).length());row['last_camera']=list(camera.to_tuple())
  if phase=='prepare':
   h.release();p.character_movement.stop_movement_immediately();grip=a.part.get_socket_location('Grip_One');p.set_actor_location(unreal.Vector(grip.x+48,grip.y+12,247),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=180,roll=0));p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),True)
   p.un_crouch()
   row={'name':a.get_actor_label(),'initial':a.get_opening()};samples=[];phase='settle';began=now;return
  if phase=='settle' and now-began>1.3:
   grip=a.part.get_socket_location('Grip_One');pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(p.get_actor_location(),grip));phase='aim';began=now;return
  if phase=='aim':
   grip=a.part.get_socket_location('Grip_One');r=unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),grip);r.yaw=unreal.MathLibrary.find_look_at_rotation(p.get_actor_location(),grip).yaw;pc.set_control_rotation(r)
  if phase=='aim' and now-began>1:
   row['pawn']=str(p.get_actor_location());row['shoulder']=str(p.mesh.get_socket_location('upperarm_r'));row['target']=str(a.part.get_socket_location('Grip_One'));row['shoulder_distance']=(p.mesh.get_socket_location('upperarm_r')-a.part.get_socket_location('Grip_One')).length();row['camera_before']=list(pc.player_camera_manager.get_camera_location().to_tuple());row['grab']=h.try_grab(True);row['hint']=h.hint;row['held']=h.held.get_owner().get_actor_label() if h.held else None
   if h.held!=a.part:row['grab']=False;report.append(row);idx+=1;phase='prepare'
   else:phase='hold';began=now
   if idx>=len(props):finish()
   return
  if phase=='hold':
   if not h.held:row['lost']=h.hint;row['grab']=False;phase='done'
   elif now-began>1:
    row['camera_hold']=list(pc.player_camera_manager.get_camera_location().to_tuple());row['camera_grab_shift']=(pc.player_camera_manager.get_camera_location()-unreal.Vector(*row['camera_before'])).length();row['grip_error']=(p.mesh.get_socket_location('hand_r')-h.contact_goals[1]).length();h.drag_interaction(9 if a.sliding else 12);phase='drag';began=now
   return
  if phase=='drag':
   shoulder=p.mesh.get_socket_location('upperarm_r');elbow=p.mesh.get_socket_location('lowerarm_r');wrist=p.mesh.get_socket_location('hand_r');extension=(wrist-shoulder).length()/max(.01,(elbow-shoulder).length()+(wrist-elbow).length());row['max_arm_extension']=max(row.get('max_arm_extension',0),extension)
   row['max_camera_drag_shift']=max(row.get('max_camera_drag_shift',0),(pc.player_camera_manager.get_camera_location()-unreal.Vector(*row['camera_hold'])).length());samples.append((p.mesh.get_socket_location('hand_r')-h.contact_goals[1]).length())
   if now-began>1.5:row['dragged']=a.get_opening()-row['initial'];row['max_wrist_error']=max(samples);row['still_held']=h.held is not None;row['carry_speed']=h.carry_speed_scale;h.release();row['look_restored']=not pc.is_look_input_ignored();row['tag_cleared']=not p.actor_has_tag('AltaiFurnitureGrip');phase='close';began=now
  if phase=='close':
   a.drive_grip(0,12000)
   if now-began<2:return
   phase='done'
  if phase=='done':
   report.append(row);idx+=1
   if idx>=len(props):finish();return
   phase='prepare'
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
