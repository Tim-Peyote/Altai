"""Targeted corrections after first render; preserves the level layout."""
import unreal,json,math,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_landform import height
R='/Game/Altai/Environment';EA=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary
ACT=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);LEVEL=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if 'L_CharacterLab' not in world.get_path_name():raise RuntimeError('Open character lab first')
m=unreal.load_asset(R+'/Materials/M_ValleyLandscape')
for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom):
 if e.get_outer()==m and 'return float3(P.xy/350,0)' in e.get_editor_property('code'):
  e.set_editor_property('code','return P.xy/350;');e.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
ME.recompile_material(m);EA.save_loaded_asset(m)
actors=ACT.get_all_level_actors();report={}
for a in actors:
 if isinstance(a,unreal.AltaiWeatherRig):
  cloudmat=a.clouds.get_editor_property('material');report['cloud_parameters']=[str(n) for n in ME.get_scalar_parameter_names(cloudmat)]
 if a.get_actor_label()=='Shallow_Basin':
  b=a.static_mesh_component.static_mesh.get_bounding_box();report['water_bounds']=str(b)
  # Imported OBJ without UVs is replaced below, with explicit indexed UVs.
 if isinstance(a,unreal.TextRenderActor):
  c=a.get_component_by_class(unreal.TextRenderComponent);c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
  c.set_world_size(16)

# Replace only our water mesh with explicit UV/normal data required by the OBJ importer.
p=Path(unreal.Paths.project_dir(),'SourceArt/Environment/ShallowWater.obj');count=64
verts=[(0,0,0)]+[(math.cos(i/count*math.tau),math.sin(i/count*math.tau),0) for i in range(count)]
lines=['o ShallowWater']+['v %f %f %f'%v for v in verts]+['vt %f %f'%((v[0]+1)/2,(v[1]+1)/2) for v in verts]+['vn 0 0 1']
lines+=['f 1/1/1 %d/%d/1 %d/%d/1'%(i+2,i+2,(i+1)%count+2,(i+1)%count+2) for i in range(count)]
p.write_text('\n'.join(lines))
t=unreal.AssetImportTask();t.filename=str(p);t.destination_path=R+'/Geometry';t.destination_name='SM_ShallowWater';t.automated=True;t.replace_existing=True;t.save=True;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
for a in actors:
 if a.get_actor_label()=='Shallow_Basin':
  mesh=a.static_mesh_component.static_mesh;b=mesh.get_bounding_box();report['water_bounds_after']=str(b)
  size=[b.max.x-b.min.x,b.max.y-b.min.y,b.max.z-b.min.z]
  if size[1]<.01:
   a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=0,roll=90),False);a.set_actor_scale3d(unreal.Vector(1500,1,1100))
  else:a.set_actor_scale3d(unreal.Vector(1500,1100,1))
 # Low hidden collision trunks should also stay invisible in the editor view.
 if a.get_actor_label().startswith('TrunkCollision'):
  a.set_is_temporarily_hidden_in_editor(True)

# Complete the camp table support.
wood=unreal.load_asset(R+'/Scans/dead_tree_trunk/M_dead_tree_trunk')
labels={a.get_actor_label() for a in actors}
for i,(x,y) in enumerate([(4270,-3650),(4330,-3650),(4270,-3450),(4330,-3450)]):
 if 'TableSupport_%d'%i in labels:continue
 a=ACT.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,height(4300,-3550)+40));a.set_actor_label('TableSupport_%d'%i);a.set_folder_path('08_Camp');a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));a.static_mesh_component.set_material(0,wood);a.set_actor_scale3d(unreal.Vector(.12,.12,.8))
LEVEL.save_current_level();Path(unreal.Paths.project_saved_dir(),'lab_polish_report.json').write_text(json.dumps(report,indent=2))
unreal.log('ALTAI_LAB_POLISHED')
