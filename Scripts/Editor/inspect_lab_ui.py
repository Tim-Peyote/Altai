import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context()
Path(unreal.Paths.project_saved_dir(),'lab_ui_state.json').write_text(json.dumps({'screen':str(pc.current_screen),'developer':pc.developer_panel is not None,'paused':unreal.GameplayStatics.is_game_paused(w),'move_blocked':pc.is_move_input_ignored()},indent=2))
