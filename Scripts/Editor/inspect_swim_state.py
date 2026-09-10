import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);s=pc.swimming
Path(unreal.Paths.project_saved_dir(),'swim_state.json').write_text(json.dumps({'state':str(s.state),'phase':s.phase,'alpha':s.swim_alpha,'p':str(p.get_actor_location()),'actor_rot':str(p.get_actor_rotation()),'control':str(pc.get_control_rotation()),'camera':str(pc.player_camera_manager.get_camera_location()),'camera_rot':str(pc.player_camera_manager.get_camera_rotation()),'neck':str(p.mesh.get_socket_location('neck_01')),'head':str(p.mesh.get_socket_location('head')),'fps':p.first_person},indent=2))
