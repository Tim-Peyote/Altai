import unreal,json
from pathlib import Path
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();p=unreal.GameplayStatics.get_player_pawn(w,0);out={}
for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor):
 if 'Low_Obstacle' in a.get_actor_label():out[a.get_actor_label()]={'pos':str(a.get_actor_location()),'scale':str(a.get_actor_scale3d()),'collision':str(a.static_mesh_component.get_collision_enabled()),'profile':str(a.static_mesh_component.get_collision_profile_name())}
for y in [-2300,-2250,-2200,-2175]:
 floor=unreal.SystemLibrary.line_trace_single(w,unreal.Vector(3300,y,1000),unreal.Vector(3300,y,-1000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[p],unreal.DrawDebugTrace.NONE,True)
 out[str(y)+'floor']=str(floor.to_tuple()) if floor else None
 for z in [100,120,140]:
  hit=unreal.SystemLibrary.line_trace_single(w,unreal.Vector(3300,y,z),unreal.Vector(3300,y+140,z),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[p],unreal.DrawDebugTrace.NONE,True)
  out[str((y,z))]=str(hit.to_tuple()) if hit else None
Path(unreal.Paths.project_saved_dir(),'stumble_diagnostic.json').write_text(json.dumps(out,indent=2))
