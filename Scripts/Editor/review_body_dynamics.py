"""Capture actual Chaos body movement, transitions, and recovery after a side impact."""
import unreal,time,json,traceback,math
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);body=pc.body_dynamics
unreal.AltaiEditorLibrary.set_physics_test_mode(True);pc.hands.release();pc.wall_climbing.release_wall();pc.traversal.cancel_climb();p.un_crouch()
names=['spine_01','spine_02','spine_03','spine_04','spine_05','neck_01','neck_02','head','clavicle_l','clavicle_r','pelvis','thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r','upperarm_l','lowerarm_l','hand_l','upperarm_r','lowerarm_r','hand_r']
fixture=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Throw_Target_1kg');fc=fixture.static_mesh_component
old_fixture=(fixture.get_actor_transform(),fc.static_mesh,fc.is_simulating_physics(),fc.get_collision_profile_name());old_pawn=p.get_actor_transform()
recoveries_before=body.recoveries
fc.set_simulate_physics(False);fc.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));fc.set_collision_profile_name('BlockAll');fixture.set_actor_scale3d(unreal.Vector(60,60,.2));fixture.set_actor_location(unreal.Vector(9000,-1800,3390),False,True)
fixture.set_actor_rotation(unreal.Rotator(),True)
p.set_actor_location(unreal.Vector(9000,-1800,3496),False,True);p.set_actor_rotation(unreal.Rotator(),True);p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING);p.character_movement.stop_movement_immediately()
rows=[];began=time.monotonic();triggered=False

def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 if body.state==unreal.AltaiBodyState.BALANCED:
  p.set_actor_transform(old_pawn,False,True);fc.set_static_mesh(old_fixture[1]);fixture.set_actor_transform(old_fixture[0],False,True);fc.set_collision_profile_name(old_fixture[3]);fc.set_simulate_physics(old_fixture[2])
 states=sorted(set(r['state'] for r in rows));checks={'physical_fall':any('FALLING' in s for s in states),'recovery_started':any('GETTING_UP' in s for s in states),'recovery_finished':body.recoveries>recoveries_before,'movement_restored':not pc.is_move_input_ignored(),'finite':all(all(math.isfinite(v) for v in xyz) for r in rows for xyz in r['bones'].values())}
 Path(unreal.Paths.project_saved_dir(),'body_dynamics_review.json').write_text(json.dumps({'passed':all(checks.values()) and not error,'mesh_class':p.mesh.get_class().get_name(),'hint':body.hint,'checks':checks,'states':states,'error':error,'samples':rows},indent=2))
def tick(dt):
 global triggered
 try:
  elapsed=time.monotonic()-began
  if elapsed<1:return
  if not triggered:triggered=True;body.test_fall()
  rows.append({'t':elapsed,'state':str(body.state),'progress':body.recovery_progress,'face_up':body.face_up,'acquire_duration':body.recovery_acquire,'bones':{n:list(p.mesh.get_socket_location(n).to_tuple()) for n in names},'rotations':{n:list(p.mesh.get_socket_rotation(n).quaternion().to_tuple()) for n in names},'angular_velocity':list(body.angular_velocity.to_tuple()),'mass':body.physical_mass,'camera':list(pc.player_camera_manager.get_camera_location().to_tuple())})
  if elapsed>18 or (elapsed>4 and body.recoveries>recoveries_before):finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
