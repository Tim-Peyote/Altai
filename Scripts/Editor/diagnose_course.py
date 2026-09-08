import unreal
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0)
unreal.log('COURSE_STEP_HEIGHT '+str(p.character_movement.max_step_height))
for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor):
 c,e=a.get_actor_bounds(False)
 if abs(c.x-5400)<300 and abs(c.y+3600)<400:unreal.log('COURSE_NEAR '+a.get_actor_label()+' '+str(c)+' '+str(e))
