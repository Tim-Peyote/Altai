"""Review actual wall attachment after fixture repositioning has settled."""
import unreal,time,json,sys,traceback
from pathlib import Path
from altai_lab_tools import context,AltaiLabTools
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.wall_climbing;t=pc.traversal
unreal.AltaiEditorLibrary.set_physics_test_mode(True);unreal.GameplayStatics.set_global_time_dilation(w,1);p.custom_time_dilation=1
pc.hands.release();t.cancel_climb();wall.release_wall();AltaiLabTools.prepare_wall_height('RoughRock',250);wall.release_wall();p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FLYING);p.character_movement.stop_movement_immediately();p.un_crouch();wall.input_from_player=False
names=['thigh_l','thigh_r','calf_l','calf_r','foot_l','foot_r','upperarm_l','upperarm_r','lowerarm_l','lowerarm_r','hand_l','hand_r'];rows=[];began=time.monotonic();started=False

def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 sys.path.insert(0,str(Path(unreal.Paths.project_dir())/'Scripts/MCP'));from analyze_climbing_joints import analyze
 result=analyze(rows);result['error']=error;result['passed']=result['passed'] and not error;result['samples']=rows
 Path(unreal.Paths.project_saved_dir(),'climbing_entry_review.json').write_text(json.dumps(result,indent=2))
def tick(dt):
 global started
 try:
  elapsed=time.monotonic()-began
  if elapsed<1:return
  if not started:
   started=wall.attach_wall()
   if not started:raise RuntimeError('Fixture could not attach')
  rows.append({'phase':'entry','progress':wall.contact_progress,'attached':wall.attached,'mantling':False,'bones':{b:list(p.mesh.get_socket_location(b).to_tuple()) for b in names},'rotations':{b:list(p.mesh.get_socket_rotation(b).quaternion().to_tuple()) for b in names}})
  if elapsed>4:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
