"""Thirty-second third-person shore traversal; restores all temporary controls."""
import unreal,time,builtins
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);s=pc.swimming
s.reset_at_shore();s.input_from_player=False;p.set_first_person(False)
pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=90,roll=0))
start=time.monotonic()
def tick(dt):
 elapsed=time.monotonic()-start
 if elapsed<12:p.add_movement_input(unreal.Vector(-1,0,0),1,False)
 elif elapsed<16:pass
 elif elapsed<32:p.add_movement_input(unreal.Vector(1,0,0),1,False)
 else:
  unreal.unregister_slate_post_tick_callback(handle);s.input_from_player=True;s.reset_at_shore()
handle=unreal.register_slate_post_tick_callback(tick)
builtins._altai_shore_view_handle=handle
