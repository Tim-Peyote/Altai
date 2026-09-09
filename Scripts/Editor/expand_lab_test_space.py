"""Update only authored lab fixtures. Saved map backup lives in Saved/Backups/LabExpansion."""
import unreal,json,math,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_landform import height
from lab_test_geometry import make_all
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);E=unreal.EditorAssetLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();ME=unreal.MaterialEditingLibrary
R='/Game/Altai/Environment';F=Path(unreal.Paths.project_dir(),'SourceArt/Environment/FieldTests');make_all(F)
D=json.loads(Path(unreal.Paths.project_saved_dir(),'environment_assets.json').read_text())
existing={a.get_actor_label():a for a in A.get_all_level_actors()}
def save(a):E.save_loaded_asset(a)
def spawn(cls,name,pos,folder,rot=None):
 a=existing.get(name) or A.spawn_actor_from_class(cls,unreal.Vector(*pos),rot or unreal.Rotator())
 a.set_actor_label(name);a.set_folder_path(folder);a.set_actor_location(unreal.Vector(*pos),False,True)
 if rot:a.set_actor_rotation(rot,False)
 existing[name]=a;return a
def mesh(name,path,pos,scale,folder,mat=None,collision='BlockAll',rot=None):
 a=spawn(unreal.StaticMeshActor,name,pos,folder,rot);c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset(path));a.set_actor_scale3d(unreal.Vector(*scale));c.set_collision_profile_name(collision)
 if mat:c.set_material(0,mat)
 return a
def box(name,pos,size,folder,mat=None,collision='BlockAll',rot=None):return mesh(name,'/Engine/BasicShapes/Cube',pos,tuple(v/100 for v in size),folder,mat,collision,rot)
def text(name,words,pos,size=12,yaw=0,folder='09_Wayfinding'):
 a=spawn(unreal.TextRenderActor,name,pos,folder,unreal.Rotator(pitch=0,yaw=yaw,roll=0));a.text_render.set_text(words);a.text_render.set_world_size(size);a.text_render.set_text_render_color(unreal.Color(221,213,179,255));return a
# New original meshes; re-use saved versions on subsequent runs.
meshes={}
for name in ['SM_FieldBucket','SM_FieldStick','SM_FieldCrate','SM_FieldStone','SM_ClimbingEscarpment']:
 path=R+'/Geometry/'+name
 if not E.does_asset_exist(path):
  t=unreal.AssetImportTask();t.filename=str(F/(name+'.obj'));t.destination_path=R+'/Geometry';t.destination_name=name;t.automated=True;t.save=True;AT.import_asset_tasks([t])
  if not E.does_asset_exist(path):raise RuntimeError('Mesh import failed: '+name+str(t.imported_object_paths))
 meshes[name]=unreal.load_asset(path)
 SM=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
 if name!='SM_ClimbingEscarpment' and SM.get_simple_collision_count(meshes[name])==0 and SM.get_convex_collision_count(meshes[name])==0:SM.add_simple_collisions(meshes[name],unreal.ScriptingCollisionShapeType.NDOP26)
 save(meshes[name])
# Authored grasp frames: X follows fingers, Z faces the palm into the grip.
def socket(m,name,pos,finger,palm):
 s=m.find_socket(name)
 if not s:s=unreal.new_object(unreal.StaticMeshSocket,outer=m);s.set_editor_property('socket_name',name);m.add_socket(s)
 s.set_editor_property('relative_location',unreal.Vector(*pos));s.set_editor_property('relative_rotation',unreal.MathLibrary.make_rot_from_xz(unreal.Vector(*finger),unreal.Vector(*palm)))
for name in ['SM_FieldBucket','SM_FieldCrate']:
 m=meshes[name];bucket=name=='SM_FieldBucket';z=42 if bucket else 23;y=24 if bucket else 25;direction=(0,0,-1 if bucket else 1);socket(m,'Grip_L',(0,-y,z),direction,(0,1,0));socket(m,'Grip_R',(0,y,z),direction,(0,-1,0));save(m)
socket(meshes['SM_FieldBucket'],'Grip_One',(5,0,68),(0,0,-1),(-1,0,0));save(meshes['SM_FieldBucket'])
socket(meshes['SM_FieldStick'],'Grip_One',(0,0,8),(1,0,0),(0,0,-1));save(meshes['SM_FieldStick'])
# Tiled, compact materials; existing CC0 scan sources are reused.
def material(name,kind,tint=(1,1,1),rough=.8,metal=0):
 p=R+'/Materials/'+name;m=unreal.load_asset(p) or AT.create_asset(name,R+'/Materials',unreal.Material,unreal.MaterialFactoryNew());ME.delete_all_material_expressions(m)
 def node(cls,**props):
  n=ME.create_material_expression(m,getattr(unreal,cls))
  for k,v in props.items():n.set_editor_property(k,v)
  return n
 def link(a,b,p,out=''):ME.connect_material_expressions(a,out,b,p)
 def out(a,p,o=''):ME.connect_material_property(a,o,getattr(unreal.MaterialProperty,p))
 wet=node('MaterialExpressionScalarParameter',parameter_name='Wetness',default_value=0.)
 tintn=node('MaterialExpressionVectorParameter',parameter_name='Tint',default_value=unreal.LinearColor(*tint,1))
 if kind=='metal':
  col=node('MaterialExpressionConstant3Vector',constant=unreal.LinearColor(.16,.19,.17,1));out(col,'MP_BASE_COLOR')
 else:
  ident='rock_face_03' if kind=='rock' else 'forest_floor'
  tx=node('MaterialExpressionTextureSample',texture=unreal.load_asset(D['textures'][ident]['Diffuse']))
  if kind=='rock':
   pos=node('MaterialExpressionWorldPosition');uv=node('MaterialExpressionCustom',code='return P.yz/300;',output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT2);inp=unreal.CustomInput();inp.set_editor_property('input_name','P');uv.set_editor_property('inputs',[inp]);link(pos,uv,'P');link(uv,tx,'UVs')
  else:
   uv=node('MaterialExpressionTextureCoordinate',u_tiling=.25,v_tiling=4.);link(uv,tx,'UVs')
  mul=node('MaterialExpressionMultiply');link(tx,mul,'A','RGB');link(tintn,mul,'B')
  fade=node('MaterialExpressionLinearInterpolate',const_a=1.,const_b=.48);link(wet,fade,'Alpha');col=node('MaterialExpressionMultiply');link(mul,col,'A');link(fade,col,'B');out(col,'MP_BASE_COLOR')
  if kind=='rock':
   n=node('MaterialExpressionTextureSample',texture=unreal.load_asset(D['textures'][ident]['nor_dx']),sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL);link(uv,n,'UVs');out(n,'MP_NORMAL','RGB')
 rn=node('MaterialExpressionScalarParameter',parameter_name='DryRoughness',default_value=rough);r=node('MaterialExpressionLinearInterpolate',const_b=.2);link(rn,r,'A');link(wet,r,'Alpha');out(r,'MP_ROUGHNESS');out(node('MaterialExpressionConstant',r=metal),'MP_METALLIC');ME.recompile_material(m);save(m);return m
rock=material('M_LabCliff','rock',(.75,.77,.7));wood=material('M_FieldWood','wood',(.32,.22,.12));steel=material('M_FieldSteel','metal',rough=.45,metal=.8)
stone=unreal.load_asset(R+'/Materials/M_GripStone')
# Three 16m routes: uninterrupted collision for the current planar climbing model.
for label,tag,y in [('Rough','RoughRock',-3100),('Smooth','SmoothRock',-4300),('Moss','MossyRock',-5500)]:
 base=height(3500,y)-30
 a=box('Climbing_Test_'+label,(3500,y,base+800),(120,900,1600),'03_ClimbingRock/ContactTests',rock)
 a.set_editor_property('tags',[unreal.Name('AltaiClimbable'),unreal.Name(tag)]);a.set_actor_hidden_in_game(True)
 # Visual whole-face rock skin is separate from stable, simple collision.
 sign=existing.get('Climbing_Test_'+label+'_Sign')
 if sign:A.destroy_actor(sign);existing.pop('Climbing_Test_'+label+'_Sign',None)
 text('Climbing_Test_'+label+'_Sign',label.upper()+' / 16 M\nE CLIMB - C RELEASE',(3730,y-300,height(3730,y)+145),14)
 for j,(bottom,top,profile) in enumerate([(300,550,'MossyRock' if label=='Rough' else 'RoughRock'),(950,1200,'SmoothRock' if label!='Smooth' else 'MossyRock')]):
  z=base+(bottom+top)/2
  patch=spawn(unreal.AltaiGripZone,'GripPatch_'+label+str(j),(3560,y,z),'03_ClimbingRock/Coverage')
  patch.bounds.set_box_extent(unreal.Vector(30,410,(top-bottom)/2),False);patch.set_editor_property('surface_actor',a)
  dry,wet=(.95,.65) if profile=='RoughRock' else (.65,.25) if profile=='SmoothRock' else (.8,.18)
  patch.set_editor_property('dry_grip',dry);patch.set_editor_property('wet_grip',wet);patch.set_editor_property('surface_label',profile)
  visual=mesh('GripPatch_Visual_'+label+str(j),'/Engine/BasicShapes/Plane',(3564,y,z),((top-bottom)/100,8.2,1),'03_ClimbingRock/Coverage',rock,'NoCollision',unreal.Rotator(pitch=90,yaw=0,roll=0))
  visual.set_editor_property('tags',[unreal.Name('AltaiGripVisual'),unreal.Name(profile)])
 # Base surface is visually readable all the way up, covered by smaller contrasting bands.
 skin=mesh('Climbing_Skin_'+label,'/Engine/BasicShapes/Plane',(3562,y,base+800),(16,9,1),'03_ClimbingRock/Art',rock,'NoCollision',unreal.Rotator(pitch=90,yaw=0,roll=0));skin.set_editor_property('tags',[unreal.Name('AltaiGripVisual'),unreal.Name(tag)])
 for meter in [2,4,6,8,10,12,14]:text('ClimbHeight_'+label+str(meter),str(meter)+' M',(3570,y+415,base+meter*100),10,folder='03_ClimbingRock/RouteMarks')
mesh('Climbing_Escarpment',meshes['SM_ClimbingEscarpment'].get_path_name(),(3560,-4300,35),(1,1,1),'03_ClimbingRock/Art',rock,'NoCollision')
back=box('Climbing_BackCollision',(3225,-4300,750),(640,3800,1800),'03_ClimbingRock/Collision',rock);back.set_actor_hidden_in_game(True)
# Camp stations have supports, clearance and labels visible from the approach.
box('Worktable',(4300,-3650,145),(100,590,16),'08_Camp',wood)
for i,(x,y) in enumerate([(4265,-3390),(4335,-3390),(4265,-3910),(4335,-3910)]):box('Worktable_Leg_'+str(i),(x,y,100),(12,12,90),'08_Camp',wood)
props=[('Loose_Stone_1kg','SM_FieldStone',1,-3415,166,stone,'STONE / 1 KG'),('Loose_Stone_5kg','SM_FieldStone',5,-3505,166,stone,'STONE / 5 KG'),('Loose_Stone_20kg','SM_FieldStone',20,-3595,166,stone,'STONE / 20 KG'),('Carry_Bucket_Empty_2kg','SM_FieldBucket',2,-3695,156,steel,'EMPTY BUCKET / 2 KG'),('Carry_Bucket_Ballast_12kg','SM_FieldBucket',12,-3800,156,steel,'BALLAST BUCKET / 12 KG')]
for name,geo,mass,y,z,mat,label in props:
 a=mesh(name,meshes[geo].get_path_name(),(4300,y,z),(1,1,1),'08_Camp/Physics',mat,'PhysicsActor');c=a.static_mesh_component;c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_mass_override_in_kg(unreal.Name('None'),mass,True);c.set_simulate_physics(True)
 text(name+'_Label',label,(4353,y-35,140),7.5)
# Separate lower stands for long and bulky objects.
for name,geo,mass,x,y,z,mat,label in [('Carry_Stick_08kg','SM_FieldStick',.8,4580,-4040,135,wood,'BRANCH / 0.8 KG'),('Carry_Crate_18kg','SM_FieldCrate',18,4300,-4140,115,wood,'CRATE / 18 KG'),('Carry_Crate_40kg','SM_FieldCrate',40,4300,-4380,115,wood,'40 KG / ABOVE HAND LIMIT')]:
 box(name+'_Stand',(x,y,z-12),(100,160 if 'Stick' in name else 110,20),'08_Camp/Stations',wood)
 a=mesh(name,meshes[geo].get_path_name(),(x,y,z),(1,1,1),'08_Camp/Physics',mat,'PhysicsActor');c=a.static_mesh_component;c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_mass_override_in_kg(unreal.Name('None'),mass,True);c.set_simulate_physics(True)
 text(name+'_Label',label,(x+54,y-50,z-8),8)
text('Carry_Station_Header','02 / HANDS + LOAD\nF TAKE / RELEASE - V VIEW\nMATCH BODY MASS / COMPARE THE SAME ROUTE',(4700,-3370,220),13,0)
# Narrow raised beam and stepping platforms, all with actual simple collision.
x=5400;y=-4100;base=height(x,y)
box('Balance_Beam',(x,y,base+85),(45,600,24),'10_LoadCourse',wood)
for yy in [y-230,y+230]:box('Balance_Beam_Support_'+str(yy),(x,yy,base+42),(90,24,84),'10_LoadCourse',wood)
for side in [-1,1]:
 for i in range(3):
  yy=y+side*(330+i*60);h=70-i*20;box('Balance_Step_'+str(side)+'_'+str(i),(x,yy,base+h/2),(120,60,h),'10_LoadCourse',wood)
text('Balance_Route_Sign','03 / LOAD COURSE\nEMPTY HANDS - LIGHT - HEAVY\n45 CM BEAM / START + STOP',(5570,-3650,height(5570,-3650)+175),13,0)
# Signposts lead to the playable wall; the distant mountain remains scenic/mantle terrain.
text('CampSign','ALTAI / FIELD LAB\n01 CLIFF  <  02 HANDS\n03 LOAD COURSE  >',(4808,-2800,height(4800,-2800)+205),18,0)
text('RockSign','SCENIC RIDGE\nLOW LEDGE MANTLE',( -2292,200,height(-2300,200)+205),18,0)
# Collision/asset audit before saving. Physics props must have simple hulls.
report=[]
for a in A.get_all_level_actors():
 if isinstance(a,unreal.StaticMeshActor) and (a.get_actor_label().startswith(('Carry_','Loose_Stone','Climbing_Test_'))):
  c=a.static_mesh_component
  if c.is_simulating_physics() and SM.get_simple_collision_count(c.static_mesh)==0 and SM.get_convex_collision_count(c.static_mesh)==0:raise RuntimeError('Missing simple collision '+a.get_actor_label())
  report.append({'name':a.get_actor_label(),'pos':str(a.get_actor_location()),'mass':c.get_mass() if c.is_simulating_physics() else None,'bounds':str(a.get_actor_bounds(False))})
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
Path(unreal.Paths.project_saved_dir(),'lab_expansion_authored.json').write_text(json.dumps(report,indent=2))
unreal.log('LAB_TEST_SPACE_EXPANDED')
