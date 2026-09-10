"""Slow-motion visual review with automatic restoration after 50 seconds."""
import unreal,time,builtins,json
from pathlib import Path
from altai_lab_tools import context
request=Path(unreal.Paths.project_saved_dir())/'body_view_request.json'
options=json.loads(request.read_text()) if request.exists() else {}
if request.exists():request.unlink()
body_view_world,body_view_pc=context();body_view_pawn=unreal.GameplayStatics.get_player_pawn(body_view_world,0)
# Disposable fixture for a clear view, removed when PIE stops.
body_view_floor=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(body_view_world,unreal.StaticMeshActor) if a.get_actor_label()=='Throw_Target_1kg')
body_view_floor.static_mesh_component.set_simulate_physics(False);body_view_floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));body_view_floor.static_mesh_component.set_collision_profile_name('BlockAll');body_view_floor.set_actor_scale3d(unreal.Vector(60,60,.2));body_view_floor.set_actor_location(unreal.Vector(9000,-1800,3390),False,True)
body_view_floor.set_actor_rotation(unreal.Rotator(pitch=options.get('slope',0),yaw=0,roll=0),True)
body_view_pawn.set_actor_location(unreal.Vector(9000,-1800,3496),False,True);body_view_pawn.set_actor_rotation(unreal.Rotator(),True);body_view_pawn.character_movement.stop_movement_immediately()
body_view_pawn.set_first_person(False);body_view_pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=body_view_pawn.get_actor_rotation().yaw+75,roll=0));unreal.AltaiEditorLibrary.set_physics_test_mode(True)
body_view_pawn.get_component_by_class(unreal.CameraComponent).set_field_of_view(50)
unreal.GameplayStatics.set_global_time_dilation(body_view_world,options.get('dilation',.2))
if options.get('back',False):body_view_pc.body_dynamics.apply_body_impulse(unreal.Vector(-35000,0,0),body_view_pawn.mesh.get_socket_location('spine_03')+unreal.Vector(0,12,20),'spine_03')
else:body_view_pc.body_dynamics.test_fall()
body_view_started=time.monotonic();body_view_paused=False
def body_view_tick(dt):
 global body_view_started,body_view_paused
 if not body_view_paused and body_view_pc.body_dynamics.state==unreal.AltaiBodyState.GETTING_UP and body_view_pc.body_dynamics.recovery_progress>options.get('progress',.52):
  body_view_paused=True;body_view_started=time.monotonic();unreal.GameplayStatics.set_global_time_dilation(body_view_world,.001)
 if time.monotonic()-body_view_started>50:
  unreal.unregister_slate_post_tick_callback(body_view_handle);unreal.GameplayStatics.set_global_time_dilation(body_view_world,1);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
body_view_handle=unreal.register_slate_post_tick_callback(body_view_tick)
builtins._altai_body_view_handle=body_view_handle
