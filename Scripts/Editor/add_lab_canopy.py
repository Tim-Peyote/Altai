import unreal,random,math,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent));from lab_landform import height,radius
R='/Game/Altai/Environment';EA=unreal.EditorAssetLibrary;ACT=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);AT=unreal.AssetToolsHelpers.get_asset_tools()
mesh=unreal.load_asset(R+'/Scans/fir_tree_01/Meshes/fir_tree_01_c_LOD0')
for i,s in enumerate(mesh.get_editor_property('static_materials')):
 name=str(s.material_slot_name);key=next((k for k in ['twig','trunk_a','trunk_b','trunk_c','bark'] if k in name),'bark');mat=unreal.load_asset(R+'/Scans/fir_tree_01/M_fir_tree_01_'+key)
 if mat:mesh.set_material(i,mat)
if mesh.get_num_lods()<3:
 opts=unreal.StaticMeshReductionOptions();opts.auto_compute_lod_screen_size=False;vals=[]
 for pct,screen in [(1,1),(.18,.45),(.06,.2),(.015,.07)]:
  v=unreal.StaticMeshReductionSettings();v.percent_triangles=pct;v.screen_size=screen;vals.append(v)
 opts.reduction_settings=vals;unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).set_lods(mesh,opts)
EA.save_loaded_asset(mesh)
b=mesh.get_bounding_box();unreal.log('FIR_BOUNDS '+str(b))
labels={a.get_actor_label() for a in ACT.get_all_level_actors()};rng=random.Random(115)
for i,(x,y) in enumerate([(5100,-2200),(5400,-700),(4400,800),(5000,2400),(4500,3800),(2800,4000),(400,4000),(-2000,4500),(-4700,-1100),(-5100,-3300),(-3400,-4400),(-1600,-4100),(300,-4600),(2000,-4600),(5500,-4200),(4000,-4800),(3500,3200),(-2100,2400)]):
 old=next((a for a in ACT.get_all_level_actors() if a.get_actor_label()=='Mature_Fir_%02d'%i),None)
 target=rng.uniform(950,1500);scale=target/(b.max.z-b.min.z)
 a=old or ACT.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,height(x,y)-b.min.z*scale));a.set_actor_label('Mature_Fir_%02d'%i);a.set_folder_path('04_Forest/Canopy');a.static_mesh_component.set_static_mesh(mesh);a.static_mesh_component.set_collision_profile_name('NoCollision');a.static_mesh_component.set_forced_lod_model(2);a.set_actor_scale3d(unreal.Vector(scale,scale,scale));yaw=rng.uniform(0,360);a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False);ang=math.radians(yaw);cx=(b.min.x+b.max.x)/2;cy=(b.min.y+b.max.y)/2;a.set_actor_location(unreal.Vector(x-(cx*math.cos(ang)-cy*math.sin(ang))*scale,y-(cx*math.sin(ang)+cy*math.cos(ang))*scale,height(x,y)-b.min.z*scale),False,True)
for a in ACT.get_all_level_actors():
 if a.get_actor_label()=='Distant_Altai_Ridges':a.set_actor_scale3d(unreal.Vector(1,-1,1))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('LAB_CANOPY_DONE')
