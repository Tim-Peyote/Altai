"""Run in a fresh lab PIE: saturated moss must shed both hand contacts and release movement."""
import unreal, json, time
from pathlib import Path
from altai_lab_tools import context, AltaiLabTools
AltaiLabTools.prepare_wall_probe('MossyRock')
w,pc=context();wall=pc.get_editor_property('wall_climbing')
surface=next(a.get_component_by_class(unreal.AltaiWetSurface) for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.actor_has_tag('MossyRock'))
AltaiLabTools.set_lab_weather(2,14)
started=time.monotonic();samples=[];attached_once=False
def poll(dt):
 global attached_once
 if not attached_once:
  if surface.get_editor_property('wetness')<.98 and time.monotonic()-started<40:return
  AltaiLabTools.prepare_wall_probe('MossyRock')
  attached_once=wall.attach_wall()
 state={k:str(wall.get_editor_property(k)) for k in ['attached','slips','grip','stamina']}
 samples.append(state)
 if state['attached']=='False' or time.monotonic()-started>50:
  unreal.unregister_slate_post_tick_callback(handle)
  p=unreal.GameplayStatics.get_player_pawn(w,0)
  passed=state['attached']=='False' and int(state['slips'])>=2 and not pc.is_move_input_ignored()
  result={'passed':passed,'last':state,'move_input_ignored':pc.is_move_input_ignored(),'movement_mode':str(p.character_movement.movement_mode),'samples':samples}
  Path(unreal.Paths.project_saved_dir(),'wall_fall_validation.json').write_text(json.dumps(result,indent=2))
  if passed:unreal.log('WALL_FALL_VALIDATION_PASSED')
  else:unreal.log_error('WALL_FALL_VALIDATION_FAILED')
handle=unreal.register_slate_post_tick_callback(poll)
