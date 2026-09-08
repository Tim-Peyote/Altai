import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.hands;h.release();p.set_first_person(True);p.crouch();pc.set_editor_property('show_lab_hud',False)
req=json.loads(Path(unreal.Paths.project_saved_dir(),'furniture_pose_request.json').read_text());a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.AltaiArticulatedProp) if a.get_actor_label()==req['name']);phase=0;start=time.monotonic()
def tick(dt):
 global phase,start
 try:
  t=time.monotonic()-start;grip=a.part.get_socket_location('Grip_One')
  if phase==0 and t>.8:
   p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(grip.x+48,grip.y+12,197),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-10,yaw=180,roll=0));phase=1;start=time.monotonic()
  elif phase==1:
   r=unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),grip);r.yaw=unreal.MathLibrary.find_look_at_rotation(p.get_actor_location(),grip).yaw;pc.set_control_rotation(r)
   if t>1.8:
    unreal.log('FURNITURE_POSE_GRAB '+str(h.try_grab(True))+' '+h.hint);phase=2;start=time.monotonic()
  elif phase==2 and t>1:
   h.drag_interaction(9 if a.sliding else 12)
   if not req.get('first_person',True):p.set_first_person(False);pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=160,roll=0))
   unreal.unregister_slate_post_tick_callback(handle);unreal.log('FURNITURE_POSE_READY')
 except Exception:
  unreal.unregister_slate_post_tick_callback(handle);unreal.log_error(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
