import unreal,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
import lab_test_geometry
# Reload this plain Python geometry helper after edits; no reflected classes here.
import importlib;importlib.reload(lab_test_geometry)
from lab_landform import height
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);E=unreal.EditorAssetLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();ME=unreal.MaterialEditingLibrary
R='/Game/Altai/Environment';D=json.loads(Path(unreal.Paths.project_saved_dir(),'environment_assets.json').read_text());actors={a.get_actor_label():a for a in A.get_all_level_actors()}
folder=Path(unreal.Paths.project_dir(),'SourceArt/Environment/FieldTests');lab_test_geometry.make_all(folder)
t=unreal.AssetImportTask();t.filename=str(folder/'SM_ClimbingEscarpment.obj');t.destination_path=R+'/Geometry';t.destination_name='SM_ClimbingEscarpment';t.replace_existing=True;t.automated=True;t.save=True;AT.import_asset_tasks([t])
parent=unreal.load_asset(R+'/Materials/M_LabCliff')
for tag,col,rough in [('RoughRock',(.68,.7,.62),.85),('SmoothRock',(.37,.43,.45),.38),('MossyRock',(.24,.39,.12),.96)]:
 name='MI_Lab_'+tag;m=unreal.load_asset(R+'/Materials/'+name) or AT.create_asset(name,R+'/Materials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew());ME.set_material_instance_parent(m,parent);ME.set_material_instance_vector_parameter_value(m,'Tint',unreal.LinearColor(*col,1));ME.set_material_instance_scalar_parameter_value(m,'DryRoughness',rough);E.save_loaded_asset(m)
 for a in actors.values():
  if isinstance(a,unreal.StaticMeshActor) and a.actor_has_tag(tag) and a.actor_has_tag('AltaiGripVisual'):a.static_mesh_component.set_material(0,m)
# Existing scan chunks round out the skyline and tie the facade to the surrounding geology.
for i,(y,sz,sy) in enumerate([(-5960,450,700),(-5200,600,1000),(-4380,500,1100),(-3470,580,1100),(-2700,420,600)]):
 info=D['meshes']['rock_moss_set_01'][i%len(D['meshes']['rock_moss_set_01'])];mn=info['min'];mx=info['max'];sx=650;scale=[sx/(mx[0]-mn[0]),sy/(mx[1]-mn[1]),sz/(mx[2]-mn[2])]
 pos=(3540-mx[0]*scale[0],y-(mn[1]+mx[1])*.5*scale[1],1700-mn[2]*scale[2]);name='Climbing_Crown_'+str(i)
 a=actors.get(name) or A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*pos));a.set_actor_label(name);a.set_folder_path('03_ClimbingRock/Art');a.set_actor_location(unreal.Vector(*pos),False,True);a.static_mesh_component.set_static_mesh(unreal.load_asset(info['path']));a.set_actor_scale3d(unreal.Vector(*scale));a.static_mesh_component.set_collision_profile_name('BlockAll')
# Keep the sightline and the beam free from random dressing.
for a in actors.values():
 p=a.get_actor_location();name=a.get_actor_label()
 if name.startswith(('Pine_','Pine_Undergrowth_','Moss_Boulder_')) and ((3900<p.x<5900 and -4800<p.y<-3200) or (3550<p.x<4050 and -6200<p.y<-2700)):
  a.set_actor_location(unreal.Vector(p.x+1100,p.y,p.z+height(p.x+1100,p.y)-height(p.x,p.y)),False,True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('EXPANDED_SPACE_POLISHED')
