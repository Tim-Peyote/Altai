import unreal
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0)
p.set_first_person(False);pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=110,roll=0))
