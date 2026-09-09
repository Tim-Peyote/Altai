"""First-person view aimed at the footholds to check eye/chest clearance."""
import unreal,time
from altai_lab_tools import context,AltaiLabTools
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.wall_climbing;t=pc.traversal
p.custom_time_dilation=1;t.cancel_climb();wall.release_wall();pc.hands.release();p.un_crouch();unreal.AltaiEditorLibrary.set_physics_test_mode(True);AltaiLabTools.prepare_wall_height('RoughRock',100);p.set_first_person(True);pc.show_lab_hud=True
began=time.monotonic()
def tick(dt):
 if time.monotonic()-began<1.5:return
 pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),pc.hands.contact_goals[3]));unreal.WidgetLibrary.set_input_mode_game_only(pc,True);unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
