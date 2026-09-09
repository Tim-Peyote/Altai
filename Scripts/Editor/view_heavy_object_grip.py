import unreal,sys
from pathlib import Path
from altai_lab_tools import context
sys.path.insert(0,str(Path(__file__).parent));from lab_landform import height
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.hands;h.release();p.un_crouch();p.set_first_person(True)
a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Carry_Bucket_Ballast_12kg');c=a.static_mesh_component
p.set_actor_location(unreal.Vector(a.get_actor_location().x-65,a.get_actor_location().y,height(a.get_actor_location().x-65,a.get_actor_location().y)+101),False,True);p.set_actor_rotation(unreal.Rotator(),True);p.character_movement.stop_movement_immediately();pc.set_control_rotation(unreal.Rotator(pitch=-30,yaw=0,roll=0));pc.show_lab_hud=True
unreal.AltaiEditorLibrary.set_physics_test_mode(True)
import time
began=time.monotonic();phase=0
def tick(dt):
 global phase
 if time.monotonic()-began<1:return
 if time.monotonic()-began<1.8:pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),c.get_center_of_mass()));return
 h.try_grab(False);pc.set_control_rotation(unreal.Rotator(pitch=-50,yaw=0,roll=0));unreal.WidgetLibrary.set_input_mode_game_only(pc,True);unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
