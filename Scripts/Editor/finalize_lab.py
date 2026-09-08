import unreal,sys,math,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent));from lab_landform import height
EA=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary;ACT=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);R='/Game/Altai/Environment'
actors=ACT.get_all_level_actors();labels={a.get_actor_label() for a in actors}
mat=unreal.load_asset(R+'/Materials/M_DistantRidges')
for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom):
 if e.get_outer()==mat:
  c=e.get_editor_property('code');c=c.replace('smoothstep(1600,2600,P.z)','smoothstep(4500,8500,P.z)').replace('smoothstep(.12,.52,1-abs(N.z))','smoothstep(.025,.22,1-abs(N.z))');e.set_editor_property('code',c)
ME.recompile_material(mat);EA.save_loaded_asset(mat)
# A material carries its physical surface identity into traces on detailed scans.
for folder,kind in [('rock_face_01','Stone'),('rock_moss_set_01','Stone'),('dead_tree_trunk','Wood'),('pine_sapling_small','Wood'),('fir_tree_01','Wood')]:
 phys=unreal.load_asset(R+'/Surfaces/PM_'+kind)
 if not phys:continue
 for path in EA.list_assets(R+'/Scans/'+folder,recursive=False):
  material=unreal.load_asset(path)
  if isinstance(material,unreal.Material):material.set_editor_property('phys_material',phys);EA.save_loaded_asset(material)
# Trunk collision is a simple separate primitive: branches do not block the character.
for a in actors:
 if not a.get_actor_label().startswith('Mature_Fir_'):continue
 label=a.get_actor_label()+'_Trunk'
 if label in labels:continue
 m=a.static_mesh_component.static_mesh;b=m.get_bounding_box();s=a.get_actor_scale3d().z
 # Canopy is asymmetric; the root is near the centre of the horizontal scan bounds.
 local=unreal.Vector((b.min.x+b.max.x)/2,(b.min.y+b.max.y)/2,b.min.z)
 p=unreal.MathLibrary.transform_location(a.get_actor_transform(),local)
 c=ACT.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(p.x,p.y,p.z+220));c.set_actor_label(label);c.set_folder_path('04_Forest/Collision');c.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cylinder'));c.set_actor_scale3d(unreal.Vector(.4,.4,4.4));c.set_actor_hidden_in_game(True);c.set_is_temporarily_hidden_in_editor(True)
# Bound the playable Landscape before the decorative non-colliding background.
for i,(x,y,sx,sy) in enumerate([(7500,0,100,15000),(-7500,0,100,15000),(0,7500,15000,100),(0,-7500,15000,100)]):
 name='Lab_Boundary_%d'%i
 if name in labels:continue
 a=ACT.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,5000));a.set_actor_label(name);a.set_folder_path('09_Debug/Boundary');a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));a.set_actor_scale3d(unreal.Vector(sx/100,sy/100,120));a.set_actor_hidden_in_game(True);a.set_is_temporarily_hidden_in_editor(True)
for a in actors:
 if isinstance(a,unreal.TextRenderActor):a.set_is_temporarily_hidden_in_editor(False)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();EA.save_loaded_assets([unreal.load_asset(R+'/Weather/NS_LabRain'),unreal.load_asset(R+'/Weather/NS_LabSnow')])
unreal.log('LAB_FINAL_SAVED')
