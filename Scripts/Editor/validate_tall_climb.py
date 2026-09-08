"""Real continuous ascent, no upward teleports after attachment. Run in fresh lab PIE."""
import unreal,json,time,traceback
from pathlib import Path
from altai_lab_tools import context,AltaiLabTools
AltaiLabTools.set_lab_weather(1,14)
w,pc=context();wall=pc.get_editor_property('wall_climbing');p=unreal.GameplayStatics.get_player_pawn(w,0)
AltaiLabTools.prepare_wall_height('RoughRock',0)
assert wall.get_editor_property('attached')
start=p.get_actor_location().z;began=time.monotonic();samples=[];last_sample=0;rest_until=0;next_rest=250;rests=0
pc.set_control_rotation(unreal.Rotator(pitch=20,yaw=180,roll=0))
def poll(dt):
 global last_sample,rest_until,next_rest,rests
 try:
  elapsed=time.monotonic()-began;gain=p.get_actor_location().z-start;attached=wall.get_editor_property('attached')
  if elapsed-last_sample>1:samples.append({'gain':gain,'stamina':wall.get_editor_property('stamina'),'grip':wall.get_editor_property('grip')});last_sample=elapsed
  if gain>=1000 or not attached or elapsed>110:
   unreal.unregister_slate_post_tick_callback(handle);wall.set_move_intent(unreal.Vector2D(0,0))
   r={'passed':gain>=1000 and attached,'gain_cm':gain,'elapsed':elapsed,'samples':samples,'rests':rests};Path(unreal.Paths.project_saved_dir(),'tall_climb_validation.json').write_text(json.dumps(r,indent=2));unreal.log('TALL_CLIMB_VALIDATION_'+('PASSED' if r['passed'] else 'FAILED'));return
  if gain>=next_rest and gain<1000:
   # Re-seat each contact near its neutral position with the production manual placement.
   for limb in range(4):
    while wall.get_editor_property('selected_limb')!=limb:wall.select_next_limb()
    target=p.get_actor_location()+p.get_actor_forward_vector()*44+p.get_actor_right_vector()*((1 if limb%2 else -1)*(28 if limb<2 else 22))+unreal.Vector(0,0,45 if limb<2 else -60)
    pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(p.get_actor_location()+unreal.Vector(0,0,50),target))
    assert wall.place_contact(), 'Manual rest contact unreachable'
   rest_until=elapsed+8;next_rest+=250;rests+=1
  if elapsed<rest_until:wall.set_move_intent(unreal.Vector2D(0,0))
  else:wall.set_move_intent(unreal.Vector2D(0,1))
 except Exception:
  unreal.unregister_slate_post_tick_callback(handle);unreal.log_error(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(poll)
