import unreal,json,math,random,sys,importlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent));from lab_landform import height,radius
EA=unreal.EditorAssetLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();ME=unreal.MaterialEditingLibrary;ACT=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);LEVEL=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
R='/Game/Altai/Environment';w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert 'L_CharacterLab' in w.get_path_name()
D=json.loads(Path(unreal.Paths.project_saved_dir(),'environment_assets.json').read_text());actors=ACT.get_all_level_actors();labels={a.get_actor_label() for a in actors}
terrain=unreal.load_asset(R+'/Materials/M_ValleyLandscape')
for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom):
 if e.get_outer()==terrain:
  code=e.get_editor_property('code')
  if 'base=lerp(base,rock,slope);' in code:
   e.set_editor_property('code',code.replace('base=lerp(base,rock,slope);','base=lerp(base*float3(.34,.43,.28),rock*.7,slope);'))
ME.recompile_material(terrain);EA.save_loaded_asset(terrain)
# Replace exposed thin scan backs with solid outcrops embedded in the sculpted mountain.
for a in actors:
 if a.get_actor_label().startswith('Cliff_Face_'):
  i=int(a.get_actor_label().split('_')[-1]);info=D['meshes']['rock_moss_set_01'][i]
  x,y=[(-3950,2300),(-4050,1200),(-3700,3400),(-4750,3100)][i];target=[1300,950,1100,1500][i]
  size=target/(info['max'][2]-info['min'][2]);cx=(info['max'][0]+info['min'][0])/2;cy=(info['max'][1]+info['min'][1])/2
  a.static_mesh_component.set_static_mesh(unreal.load_asset(info['path']));a.set_actor_rotation(unreal.Rotator(0,0,0),False);a.set_actor_scale3d(unreal.Vector(size,size,size))
  a.set_actor_location(unreal.Vector(x-cx*size,y-cy*size,height(x,y)-target*.45-info['min'][2]*size),False,True)
  a.set_actor_label('Cliff_Outcrop_%02d'%i)
# Distant ridges provide a coherent horizon outside the playable, sculptable Landscape.
if 'Distant_Altai_Ridges' not in labels:
 n=101;step=520;vertices=[];uvs=[];faces=[]
 for iy in range(n):
  y=(iy-50)*step
  for ix in range(n):
   x=(ix-50)*step;edge=max(abs(x),abs(y));near=height(max(-7560,min(7560,x)),max(-7560,min(7560,y)))
   peaks=1000+7500*math.exp(-((x+14500)/5500)**2-((y-4500)/8500)**2)+9500*math.exp(-((x+2000)/9500)**2-((y-17500)/4500)**2)+5600*math.exp(-((x-16000)/6500)**2-((y-14000)/5000)**2)
   erosion=(math.sin(x/950)*math.sin(y/720)+.4*math.sin((x+y)/370))*350
   blend=max(0,min(1,(edge-7350)/4300));z=near*(1-blend)+(peaks+erosion)*blend
   vertices.append((x,y,z));uvs.append((x/500,y/500))
 for y in range(n-1):
  for x in range(n-1):
   cx=(x-49.5)*step;cy=(y-49.5)*step
   if abs(cx)<7200 and abs(cy)<7200:continue
   a=y*n+x+1;b=a+1;c=a+n;d=c+1;faces.extend([(a,b,c),(b,d,c)])
 lines=['o DistantRidges']+['v %.3f %.3f %.3f'%v for v in vertices]+['vt %.4f %.4f'%v for v in uvs]+['f %d/%d %d/%d %d/%d'%(a,a,b,b,c,c) for a,b,c in faces]
 p=Path(unreal.Paths.project_dir(),'SourceArt/Environment/DistantRidges.obj');p.write_text('\n'.join(lines))
 t=unreal.AssetImportTask();t.filename=str(p);t.destination_path=R+'/Geometry';t.destination_name='SM_DistantRidges';t.automated=True;t.save=True;AT.import_asset_tasks([t])
 mesh=next(unreal.load_asset(p) for p in t.imported_object_paths if isinstance(unreal.load_asset(p),unreal.StaticMesh))
 mesh.set_material(0,terrain);EA.save_loaded_asset(mesh)
 a=ACT.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0));a.set_actor_label('Distant_Altai_Ridges');a.set_folder_path('01_Landscape/Backdrop');a.static_mesh_component.set_static_mesh(mesh);a.static_mesh_component.set_collision_profile_name('NoCollision')
# Foliage system batches repeated cover and supports native repainting/removal in Foliage Mode.
rng=random.Random(811)
for typ,info in enumerate(D['meshes']['grass_bermuda_01'][:5]):
 name='FT_GroundCover_%d'%typ;path=R+'/Foliage/'+name
 if EA.does_asset_exist(path):continue
 ft=AT.create_asset(name,R+'/Foliage',unreal.FoliageType_InstancedStaticMesh,unreal.FoliageType_InstancedStaticMeshFactory());ft.set_editor_property('mesh',unreal.load_asset(info['path']));ft.set_editor_property('cull_distance',unreal.Int32Interval(min=5000,max=7500));EA.save_loaded_asset(ft)
 transforms=[];cx=(info['min'][0]+info['max'][0])/2;cy=(info['min'][1]+info['max'][1])/2;hh=max(1,info['max'][2]-info['min'][2])
 for i in range(350):
  x=rng.uniform(-6100,6100);y=rng.uniform(-6100,6100)
  if radius(x,y,1800,1700,1650,1200)<1 or radius(x,y,-700,1300,1150,950)<1 or abs(y+3100)<180 or height(x,y)>1400:continue
  size=rng.uniform(12,30)/hh;yaw=rng.uniform(0,360);a=math.radians(yaw)
  pos=unreal.Vector(x-(cx*math.cos(a)-cy*math.sin(a))*size,y-(cx*math.sin(a)+cy*math.cos(a))*size,height(x,y)-info['min'][2]*size)
  transforms.append(unreal.Transform(location=pos,rotation=unreal.Rotator(pitch=0,yaw=yaw,roll=0),scale=unreal.Vector(size,size,size)))
 unreal.InstancedFoliageActor.add_instances(w,ft,transforms)
# A restrained default overcast state; the HUD controls the full weather matrix at runtime.
for a in ACT.get_all_level_actors():
 if isinstance(a,unreal.AltaiWeatherRig):
  a.set_editor_property('hour',15);a.set_editor_property('preset_index',1);a.preview_weather()
 if isinstance(a,unreal.TextRenderActor):a.set_is_temporarily_hidden_in_editor(True)
LEVEL.save_current_level()
unreal.log('ALTAI_ART_FINISHED')
