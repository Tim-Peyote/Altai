"""Inspect a specific native recovery clip pose; all changes are PIE-only."""
import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0)
request=Path(unreal.Paths.project_saved_dir())/'body_view_request.json'
options=json.loads(request.read_text()) if request.exists() else {}
if request.exists():request.unlink()
pc.body_dynamics.enabled=False;pc.hands.release();p.set_first_person(False)
p.character_movement.disable_movement()
floor=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Throw_Target_1kg')
floor.static_mesh_component.set_simulate_physics(False);floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.static_mesh_component.set_collision_profile_name('BlockAll')
floor.set_actor_scale3d(unreal.Vector(60,60,.2));floor.set_actor_location(unreal.Vector(9000,-1800,3390),False,True)
p.set_actor_location(unreal.Vector(9000,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True)
p.tags=list(p.tags)+['AltaiBodyUnbalanced']
pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=75,roll=0))
p.get_component_by_class(unreal.CameraComponent).set_field_of_view(50)
asset=unreal.load_asset('/Game/Altai/Player/Animations/A_'+options.get('clip','GetUpSupine'))
p.mesh.play_animation(asset,False);p.mesh.set_position(asset.sequence_length*options.get('progress',.4),False);p.mesh.set_play_rate(options.get('play_rate',0))
