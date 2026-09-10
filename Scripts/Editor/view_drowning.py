"""Start a quick live death review; use reset_drowning_view.py afterwards."""
import unreal
from altai_lab_tools import context
w,pc=context();s=pc.swimming;p=unreal.GameplayStatics.get_player_pawn(w,0)
s.reset_at_shore();s.input_from_player=False;s.drowning_seconds=.5;s.set_test_oxygen(0)
p.set_actor_location(unreal.Vector(1800,1700,-150),False,True);p.character_movement.stop_movement_immediately();s.dive=True;p.set_first_person(True)
