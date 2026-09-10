"""Bounded live swimming review, restores simulation speed automatically."""
import unreal,time,builtins,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);s=pc.swimming
request=Path(unreal.Paths.project_saved_dir(),'swim_view_request.json');options=json.loads(request.read_text()) if request.exists() else {}
if request.exists():request.unlink()
s.input_from_player=False;s.sprint=False;s.dive=options.get('underwater',False);s.ascend=False
p.set_actor_location(unreal.Vector(1800,1700,-130 if s.dive else 0),False,True);p.set_actor_rotation(unreal.Rotator(),True)
p.character_movement.stop_movement_immediately();p.set_first_person(options.get('first_person',False));pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=65,roll=0));p.follow_camera.set_field_of_view(60)
if options.get('tired'):s.set_test_stamina(.1)
start=time.monotonic();paused=False
unreal.GameplayStatics.set_global_time_dilation(w,.35)
def tick(dt):
 global paused,start
 if not paused and options.get('moving',False):p.add_movement_input(unreal.Vector(0,1,0),1,False)
 if not paused and s.swim_alpha>.98 and abs(s.phase-options.get('phase',.2))<.025:
  paused=True;start=time.monotonic();s.dive=False;unreal.GameplayStatics.set_global_time_dilation(w,.001)
 if time.monotonic()-start>45:
  unreal.unregister_slate_post_tick_callback(handle);unreal.GameplayStatics.set_global_time_dilation(w,1);s.input_from_player=True
handle=unreal.register_slate_post_tick_callback(tick);builtins._altai_swim_view_handle=handle
