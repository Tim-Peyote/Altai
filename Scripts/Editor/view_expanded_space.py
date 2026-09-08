import unreal,json
from pathlib import Path
views={'camp':((4800,-3650,230),(4300,-3650,165)),'overview':((6200,-2500,650),(3450,-4300,900)),'course':((5900,-3300,470),(5400,-4100,170))}
key=json.loads(Path(unreal.Paths.project_saved_dir(),'expanded_view.json').read_text())['view'];pos,target=views[key]
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(*pos),unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*pos),unreal.Vector(*target)))
unreal.EditorLevelLibrary.editor_set_game_view(True)
