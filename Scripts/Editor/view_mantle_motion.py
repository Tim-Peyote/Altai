"""Slow motion visual review; restores time scale automatically at the end."""
import unreal,time
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.wall_climbing;t=pc.traversal
p.custom_time_dilation=1;unreal.GameplayStatics.set_global_time_dilation(w,1);t.cancel_climb();wall.release_wall();pc.hands.release();p.un_crouch();wall.input_from_player=False;unreal.AltaiEditorLibrary.set_physics_test_mode(True)
rock=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Climbing_Test_Rough');center,ext=rock.get_actor_bounds(False)
p.set_actor_location(unreal.Vector(center.x+ext.x+44,center.y,center.z+ext.z-55),False,True);p.set_actor_rotation(unreal.Rotator(yaw=180),True);pc.set_control_rotation(unreal.Rotator(yaw=180));wall.attach_wall();p.set_first_person(False);pc.show_lab_hud=True
began=time.monotonic();started=False;paused_at=0

def tick(dt):
 global started,paused_at
 if time.monotonic()-began<1:return
 if not started:
  started=True;t.try_climb_from_wall();unreal.GameplayStatics.set_global_time_dilation(w,.12);pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=95,roll=0));unreal.WidgetLibrary.set_input_mode_game_only(pc,True)
 elif t.climbing and t.progress>.58 and not paused_at:
  paused_at=time.monotonic();unreal.GameplayStatics.set_global_time_dilation(w,.001)
 elif paused_at and time.monotonic()-paused_at>30 and t.climbing:
  unreal.GameplayStatics.set_global_time_dilation(w,1)
 elif not t.climbing or time.monotonic()-began>65:
  p.custom_time_dilation=1;unreal.GameplayStatics.set_global_time_dilation(w,1);unreal.AltaiEditorLibrary.set_physics_test_mode(False);unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
