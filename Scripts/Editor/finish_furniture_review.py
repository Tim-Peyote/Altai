import unreal
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0)
pc.hands.release();p.un_crouch();pc.set_editor_property('show_lab_hud',True)
unreal.AltaiEditorLibrary.set_physics_test_mode(False)
unreal.WidgetLibrary.set_input_mode_game_only(pc,True)
