import unreal
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,unreal.AltaiWeatherRig):a.preview_weather()
unreal.EditorLevelLibrary.editor_set_game_view(True)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(5100,-2200,350),unreal.Rotator(pitch=1,yaw=150,roll=0))
