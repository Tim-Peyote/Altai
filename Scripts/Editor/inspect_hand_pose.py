import unreal,json
from pathlib import Path
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
p=unreal.GameplayStatics.get_player_pawn(w,0);r={}
for n in ['hand_l','lowerarm_l','upperarm_l','hand_r','lowerarm_r','upperarm_r']:
 r[n]={'parent':str(p.mesh.get_parent_bone(n)),'position':str(p.mesh.get_socket_location(n)),'rotation':str(p.mesh.get_socket_rotation(n))}
h=p.get_component_by_class(unreal.AltaiHands);r['goals']=[str(g) for g in h.contact_goals];r['actor']=str(p.get_actor_location());r['mesh']=str(p.mesh.get_world_transform())
Path(unreal.Paths.project_saved_dir(),'hand_pose.json').write_text(json.dumps(r,indent=2))
