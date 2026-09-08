import unreal,json
from pathlib import Path
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);ME=unreal.MaterialEditingLibrary;E=unreal.EditorAssetLibrary
m=unreal.load_asset('/Game/Altai/Environment/Materials/M_LanternGlass')
for e in unreal.ObjectIterator(unreal.MaterialExpressionConstant3Vector):
 if e.get_outer()==m:e.set_editor_property('constant',unreal.LinearColor(8000,3600,800,1))
ME.recompile_material(m);E.save_loaded_asset(m)
near=[]
for a in A.get_all_level_actors():
 name=a.get_actor_label()
 if name.startswith('Camp_TaskLight_'):a.point_light_component.set_intensity(12000)
 if isinstance(a,unreal.StaticMeshActor) and not name.startswith(('Balance','Carry','Camp','Climb','Grip','Bucket','Worktable','Shelter')):
  c,e=a.get_actor_bounds(False)
  if abs(c.x-5400)<250 and -4800<c.y<-3500:near.append({'name':name,'location':str(a.get_actor_location()),'bounds':str(c)})
Path(unreal.Paths.project_saved_dir(),'course_dressing_audit.json').write_text(json.dumps(near,indent=2))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('LAB_READABILITY_SAVED')
