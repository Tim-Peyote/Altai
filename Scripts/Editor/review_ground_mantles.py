"""PIE tests for low, waist-height and high mantles using temporary collision fixtures."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.hands;wall=pc.wall_climbing;t=pc.traversal
unreal.AltaiEditorLibrary.set_physics_test_mode(True);p.custom_time_dilation=1;t.cancel_climb();wall.release_wall();h.release();p.un_crouch();p.set_first_person(False)
actors={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor)};snapshots={};rows=[]
def fixture(label,pos,scale):
 a=actors[label];c=a.static_mesh_component
 if a not in snapshots:snapshots[a]=(a.get_actor_transform(),c.static_mesh,c.is_simulating_physics(),c.get_mass(),list(a.tags))
 c.set_simulate_physics(False);c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_collision_profile_name('BlockAll');a.set_actor_scale3d(unreal.Vector(*scale));a.set_actor_location(unreal.Vector(*pos),False,True);a.tags=[unreal.Name('AltaiClimbable')];return a
fixture('Throw_Target_1kg',(6400,-1800,3390),(12,12,.2))
def wait(s):
 end=time.monotonic()+s
 while time.monotonic()<end:yield
def run():
 for height in [45,100,160]:
  wall.release_wall();t.cancel_climb();p.un_crouch();fixture('Throw_Target_3kg',(6500,-1800,3400+height*.5),(3,4,height*.01));p.set_actor_location(unreal.Vector(6306,-1800,3498),False,True);p.set_actor_rotation(unreal.Rotator(yaw=0),True);pc.set_control_rotation(unreal.Rotator(yaw=0));p.character_movement.stop_movement_immediately();yield from wait(.8)
  count=t.completed_climbs;row={'height':height,'start':t.try_climb(),'hint':t.hint};rows.append(row);yield from wait(3.5)
  row.update(completed=t.completed_climbs>count,grounded=p.character_movement.is_moving_on_ground(),height_error=p.get_actor_location().z-p.capsule_component.get_scaled_capsule_half_height()-3400-height,move_restored=not pc.is_move_input_ignored())
g=run()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);t.cancel_climb();wall.release_wall()
 for a,(trans,mesh,sim,mass,tags) in snapshots.items():
  c=a.static_mesh_component;c.set_simulate_physics(False);c.set_static_mesh(mesh);a.set_actor_transform(trans,False,True);a.tags=tags;c.set_mass_override_in_kg('None',mass,True);c.set_collision_profile_name('PhysicsActor');c.set_simulate_physics(sim)
 unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 Path(unreal.Paths.project_saved_dir(),'ground_mantles_review.json').write_text(json.dumps({'passed':not error and all(r.get('completed') and r.get('grounded') and r.get('move_restored') and abs(r.get('height_error',999))<6 for r in rows),'error':error,'cases':rows},indent=2));unreal.log('GROUND_MANTLES_DONE '+error)
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
