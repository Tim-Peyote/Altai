import unreal
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if w:
 pc=unreal.GameplayStatics.get_player_controller(w,0);pc.reset_ignore_look_input();pc.set_ignore_look_input(True)
