"""Check a rail against an external blocker, then carry a loose stone in its open box."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();pc.hands.release();p=unreal.GameplayStatics.get_player_pawn(w,0);p.set_actor_location(unreal.Vector(5150,-5580,248),False,True)
props={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.AltaiArticulatedProp)};d=props['Interact_Drawer_2'];others=[a for n,a in props.items() if 'Drawer' in n and a!=d]
static={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor)};block=static['Carry_Crate_40kg'];stone=static['Loose_Stone_1kg'];bp=block.get_actor_transform();sp=stone.get_actor_transform();block.static_mesh_component.set_simulate_physics(False)
# A crate overlaps the last portion of the opening path and obstructs the rail.
g=d.part.get_socket_location('Grip_One');block.set_actor_location(unreal.Vector(g.x+52,g.y,g.z-15),False,True)
start=time.monotonic();phase=0;report={};max_other=0

def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);block.set_actor_transform(bp,False,True);block.static_mesh_component.set_simulate_physics(True);stone.set_actor_transform(sp,False,True)
 report['error']=error;report['other_drawer_drift']=max_other;report['passed']=not error and 2<report.get('blocked',999)<31 and report.get('unblocked',0)>34 and report.get('stone_on_bottom',False) and max_other<1
 Path(unreal.Paths.project_saved_dir(),'furniture_collision_validation.json').write_text(json.dumps(report,indent=2));unreal.log('FURNITURE_COLLISION_TEST_DONE '+str(report['passed']))
def tick(dt):
 global phase,start,max_other
 try:
  t=time.monotonic()-start;max_other=max(max_other,max(abs(a.get_opening()) for a in others))
  if phase==0:
   d.drive_grip(36,12000)
   if t>3:report['blocked']=d.get_opening();block.set_actor_transform(bp,False,True);phase=1;start=time.monotonic()
  elif phase==1:
   d.drive_grip(36,12000)
   if t>3:
    report['unblocked']=d.get_opening();center=d.part.get_socket_location('None');stone.static_mesh_component.set_physics_linear_velocity(unreal.Vector());stone.set_actor_location(center+unreal.Vector(0,0,8),False,True);phase=2;start=time.monotonic()
  elif phase==2:
   if t>2:
    local=unreal.MathLibrary.inverse_transform_location(d.part.get_world_transform(),stone.static_mesh_component.get_center_of_mass());report['stone_local']=str(local);report['stone_on_bottom']=abs(local.y)<18 and -18<local.x<34 and -16<local.z<12;finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
