import unreal,time
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);p.set_first_person(True);start=time.monotonic()
def tick(dt):
 if pc.hands.held:
  grip=pc.hands.held.get_socket_location('Grip_One');r=unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),grip);r.yaw=unreal.MathLibrary.find_look_at_rotation(p.get_actor_location(),grip).yaw;pc.set_control_rotation(r)
 if time.monotonic()-start>2:unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
