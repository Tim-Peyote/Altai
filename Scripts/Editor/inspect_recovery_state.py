import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);b=pc.body_dynamics
Path(unreal.Paths.project_saved_dir(),'recovery_state.json').write_text(json.dumps({'state':str(b.state),'hint':b.hint,'issue':b.recovery_terrain_issue,'hip':list(p.mesh.get_socket_location('pelvis').to_tuple()),'position':list(p.get_actor_location().to_tuple())},indent=2))
