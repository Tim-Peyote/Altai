import unreal,json
from pathlib import Path
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors={a.get_actor_label():a for a in A.get_all_level_actors()};D=json.loads(Path(unreal.Paths.project_saved_dir(),'environment_assets.json').read_text())
for name,a in actors.items():
 if name.startswith(('Climbing_Skin_','GripPatch_Visual_')):a.set_actor_rotation(unreal.Rotator(pitch=-90,yaw=0,roll=0),False)
 if name.startswith('Pine_') and name[len('Pine_'):].isdigit():
  proxy=actors.get('TrunkCollision_'+name[len('Pine_'):])
  if proxy:
   p=a.get_actor_location();q=proxy.get_actor_location()
   if abs(p.x-q.x)>500:proxy.set_actor_location(unreal.Vector(q.x+1100,q.y,q.z),False,True)
for column,y in enumerate([-3700,-4900]):
 for j in range(3):
  info=D['meshes']['rock_moss_set_01'][(column*3+j)%len(D['meshes']['rock_moss_set_01'])];mn=info['min'];mx=info['max'];sc=[430/(mx[0]-mn[0]),260/(mx[1]-mn[1]),580/(mx[2]-mn[2])]
  pos=(3650-(mn[0]+mx[0])*.5*sc[0],y-(mn[1]+mx[1])*.5*sc[1],20+j*520-mn[2]*sc[2]);name='Climbing_Fracture_'+str(column)+'_'+str(j)
  a=actors.get(name) or A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*pos));a.set_actor_label(name);a.set_folder_path('03_ClimbingRock/Art');a.set_actor_location(unreal.Vector(*pos),False,True);a.static_mesh_component.set_static_mesh(unreal.load_asset(info['path']));a.set_actor_scale3d(unreal.Vector(*sc));a.static_mesh_component.set_collision_profile_name('BlockAll')
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('EXPANDED_ART_FINISHED')
