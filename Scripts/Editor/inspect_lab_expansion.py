import unreal,json
from pathlib import Path
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
r=[]
for a in A.get_all_level_actors():
 p=a.get_actor_location()
 if a.get_actor_label().startswith(('Cliff','Climbing','Loose','Worktable','Shelter','Field_Start')) or (3000<p.x<5500 and -5800<p.y<-2600):
  v={'label':a.get_actor_label(),'position':[p.x,p.y,p.z],'scale':str(a.get_actor_scale3d()),'class':a.get_class().get_name()}
  if isinstance(a,unreal.StaticMeshActor):v.update(mesh=str(a.static_mesh_component.static_mesh),bounds=str(a.get_actor_bounds(False)))
  r.append(v)
Path(unreal.Paths.project_saved_dir(),'lab_expansion_before.json').write_text(json.dumps(r,indent=2))
unreal.log('MESH_SOCKET_API '+str([v for v in dir(unreal.StaticMesh) if 'socket' in v]))
unreal.log('MESH_COLLISION_API '+str([v for v in dir(unreal.StaticMeshEditorSubsystem) if 'collision' in v]))
unreal.log('LAB_EXPANSION_INSPECTED')
