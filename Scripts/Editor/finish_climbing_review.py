"""Restore the lab after temporary climbing review helpers."""
import unreal
from altai_lab_tools import context
try:
 unreal.unregister_slate_post_tick_callback(handle)
except (NameError,RuntimeError):
 pass
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0)
unreal.GameplayStatics.set_global_time_dilation(w,1);p.custom_time_dilation=1
pc.traversal.cancel_climb();pc.wall_climbing.release_wall();pc.wall_climbing.input_from_player=True
unreal.AltaiEditorLibrary.set_physics_test_mode(False)
