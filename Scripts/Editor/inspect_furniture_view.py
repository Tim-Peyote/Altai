import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.hands
r={'actor':str(p.get_actor_location()),'rotation':str(p.get_actor_rotation()),'camera':str(pc.player_camera_manager.get_camera_location()),'look':str(pc.get_control_rotation()),'eye_offset':str(p.interaction_eye_offset),'body_offset':str(h.furniture_body_offset),'goal':str(h.contact_goals[1])}
for n in ['neck_01','head','spine_05','upperarm_r','lowerarm_r','hand_r','pelvis','foot_r','foot_l']:r[n]=str(p.mesh.get_socket_location(n))
Path(unreal.Paths.project_saved_dir(),'furniture_view.json').write_text(json.dumps(r,indent=2))
