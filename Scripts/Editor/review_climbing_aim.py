"""Verify camera-directed manual placement for every limb in both views."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context,AltaiLabTools
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.wall_climbing;h=pc.hands
unreal.AltaiEditorLibrary.set_physics_test_mode(True);pc.traversal.cancel_climb();wall.release_wall();h.release();p.un_crouch();wall.input_from_player=False;wall.assisted_stepping=False
rows=[]
def wait(s):
 end=time.monotonic()+s
 while time.monotonic()<end:yield

def run():
 AltaiLabTools.prepare_wall_height('RoughRock',100);yield from wait(1)
 for first in [True,False]:
  p.set_first_person(first);yield from wait(1.2)
  for limb in range(4):
   while wall.selected_limb!=limb:wall.select_next_limb()
   # Aim at the actual surface point beneath the existing hand/ankle contact.
   contact=unreal.Vector(*h.contact_goals[limb].to_tuple());target=contact+p.get_actor_forward_vector()*(7 if limb<2 else 16)
   for k in range(100):
    previous=pc.get_control_rotation().to_tuple();look=unreal.MathLibrary.find_look_at_rotation(pc.player_camera_manager.get_camera_location(),target);pc.set_control_rotation(look);yield
    current=look.to_tuple()
    if k>8 and max(abs((a-b+180)%360-180) for a,b in zip(current,previous))<.004:break
   grabbed=wall.place_contact();yield from wait(1)
   error=(h.contact_goals[limb]-contact).length();rows.append({'first_person':first,'limb':limb,'placed':grabbed,'aim_error_cm':error,'hint':wall.hint})
g=run()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);wall.assisted_stepping=True;unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 Path(unreal.Paths.project_saved_dir(),'climbing_aim_review.json').write_text(json.dumps({'passed':not error and len(rows)==8 and all(r['placed'] and r['aim_error_cm']<3 for r in rows),'error':error,'cases':rows},indent=2));unreal.log('CLIMBING_AIM_DONE '+error)
def tick(dt):
 try:next(g)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
