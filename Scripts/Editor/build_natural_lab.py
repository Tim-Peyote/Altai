"""Author the natural lab once; source blockout is retained as a separate map.
All geometry/materials/profiles are saved assets. No runtime world generation.
"""
import unreal, json, math, random, struct, sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_landform import height,radius
R='/Game/Altai/Environment';MAP='/Game/Altai/Debug/Maps/L_CharacterLab'
AT=unreal.AssetToolsHelpers.get_asset_tools();EA=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary
LEVEL=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);ACT=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
DATA=json.loads(Path(unreal.Paths.project_saved_dir(),'environment_assets.json').read_text())
RAND=random.Random(1809)

def save(a):EA.save_loaded_asset(a,only_if_is_dirty=False)
def node(m,kind,**props):
 n=ME.create_material_expression(m,getattr(unreal,kind),0,0)
 for k,v in props.items():n.set_editor_property(k,v)
 return n

def link(a,b,inp,out=''):ME.connect_material_expressions(a,out,b,inp)
def output(a,prop,out=''):ME.connect_material_property(a,out,getattr(unreal.MaterialProperty,prop))
def scalar(m,v):return node(m,'MaterialExpressionConstant',r=v)
def color(m,r,g,b):return node(m,'MaterialExpressionConstant3Vector',constant=unreal.LinearColor(r,g,b,1))
def material(name):
 old=unreal.load_asset(R+'/Materials/'+name)
 return old or AT.create_asset(name,R+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
def custom(m,code,inputs,out=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
 n=node(m,'MaterialExpressionCustom',code=code,output_type=out)
 arr=[]
 for name in inputs:
  i=unreal.CustomInput();i.set_editor_property('input_name',name);arr.append(i)
 n.set_editor_property('inputs',arr)
 for name,obj in inputs.items():link(obj,n,name)
 return n

def tex(m,ident,key):
 return node(m,'MaterialExpressionTextureObject',texture=unreal.load_asset(DATA['textures'][ident][key]))
MPC=unreal.load_asset(R+'/Weather/MPC_Environment')

def weather(m,name):return node(m,'MaterialExpressionCollectionParameter',collection=MPC,parameter_name=name)

# Terrain uses real PBR scan colour with world-scale mapping and softly blended wetlands.
terrain=material('M_ValleyLandscape')
if not EA.does_asset_exist(R+'/Materials/M_ValleyLandscape_Built'):
 ME.delete_all_material_expressions(terrain)
 pos=node(terrain,'MaterialExpressionWorldPosition');normal=node(terrain,'MaterialExpressionVertexNormalWS')
 ins={'P':pos,'N':normal,'Forest':tex(terrain,'forest_floor','Diffuse'),'Mud':tex(terrain,'brown_mud_03','Diffuse'),'Rock':tex(terrain,'rock_face_03','Diffuse'),'SnowTex':tex(terrain,'snow_02','Diffuse'),'Wet':weather(terrain,'Wetness'),'Cover':weather(terrain,'SnowCover')}
 code='''float2 uv=P.xy/350.0;
float mud=1-smoothstep(.85,1.25,length((P.xy-float2(-700,1300))/float2(1050,850)));
float shore=1-smoothstep(.9,1.45,length((P.xy-float2(1800,1700))/float2(1500,1100)));
mud=max(mud,shore);
float slope=smoothstep(.12,.52,1-abs(N.z));
float3 weights=pow(abs(N),4);weights/=max(.001,weights.x+weights.y+weights.z);
float3 rock=Texture2DSample(Rock,RockSampler,P.yz/500).rgb*weights.x+Texture2DSample(Rock,RockSampler,P.xz/500).rgb*weights.y+Texture2DSample(Rock,RockSampler,P.xy/500).rgb*weights.z;
float3 base=lerp(Texture2DSample(Forest,ForestSampler,uv).rgb,Texture2DSample(Mud,MudSampler,uv).rgb,mud);
base=lerp(base*float3(.34,.43,.28),rock*.7,slope);
float snowPatch=1-smoothstep(.8,1.2,length((P.xy-float2(-2600,-2200))/float2(700,500)));
float snow=max(max(snowPatch,smoothstep(1600,2600,P.z)),Cover*.92)*smoothstep(.35,.85,N.z);
float3 snowColor=Texture2DSample(SnowTex,SnowTexSampler,uv).rgb*.8;
float macro=.83+.17*sin(P.x/1800+sin(P.y/1000));
return lerp(base*macro*lerp(1,.62,Wet),snowColor,saturate(snow));'''
 out=custom(terrain,code,ins);output(out,'MP_BASE_COLOR')
 rough=custom(terrain,'return lerp(.88,.26,Wet);',{'Wet':weather(terrain,'Wetness')},unreal.CustomMaterialOutputType.CMOT_FLOAT1);output(rough,'MP_ROUGHNESS')
 uv=custom(terrain,'return P.xy/350;',{'P':pos},unreal.CustomMaterialOutputType.CMOT_FLOAT2)
 n=node(terrain,'MaterialExpressionTextureSample',texture=unreal.load_asset(DATA['textures']['forest_floor']['nor_dx']),sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
 link(uv,n,'UVs');output(n,'MP_NORMAL','RGB')
 ME.recompile_material(terrain);save(terrain)

# Decal boot print: sole/heel plus tread grooves. A visual impression, not terrain deformation.
for name,tint in [('M_FootprintMud',(.025,.019,.012)),('M_FootprintSnow',(.24,.31,.34))]:
 m=material(name);ME.delete_all_material_expressions(m);m.set_editor_property('material_domain',unreal.MaterialDomain.MD_DEFERRED_DECAL);m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
 uv=node(m,'MaterialExpressionTextureCoordinate')
 mask=custom(m,'''float2 p=UV.xy;
float sole=1-smoothstep(.88,1.0,length((p-float2(.5,.64))/float2(.32,.30)));
float heel=1-smoothstep(.85,1.0,length((p-float2(.5,.23))/float2(.27,.17)));
float tread=lerp(.25,1,step(.23,frac(p.y*14)));
return saturate(max(sole,heel)*tread*.8);''',{'UV':uv},unreal.CustomMaterialOutputType.CMOT_FLOAT1)
 output(mask,'MP_OPACITY');output(color(m,*tint),'MP_BASE_COLOR');output(scalar(m,.8),'MP_ROUGHNESS');ME.recompile_material(m);save(m)

water=material('M_ShallowWater');ME.delete_all_material_expressions(water)
water.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);water.set_editor_property('two_sided',True)
output(color(water,.025,.065,.07),'MP_BASE_COLOR');output(scalar(water,.14),'MP_ROUGHNESS');output(scalar(water,.6),'MP_SPECULAR')
fade=node(water,'MaterialExpressionDepthFade',fade_distance_default=90,opacity_default=.76);output(fade,'MP_OPACITY')
n=custom(water,'return normalize(float3(sin(P.x*.018+T*1.4)*.10,cos(P.y*.025+T*1.1)*.08,1));',{'P':node(water,'MaterialExpressionWorldPosition'),'T':node(water,'MaterialExpressionTime')});output(n,'MP_NORMAL');ME.recompile_material(water);save(water)

ripple=material('M_WaterRipple');ME.delete_all_material_expressions(ripple)
ripple.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);ripple.set_editor_property('two_sided',True)
ripple.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
mask=custom(ripple,'float r=length(UV.xy-.5)*2;return pow(saturate(1-abs(frac(r*2-T*1.4)-.5)*10),3)*(1-r)*.3;',{'UV':node(ripple,'MaterialExpressionTextureCoordinate'),'T':node(ripple,'MaterialExpressionTime')},unreal.CustomMaterialOutputType.CMOT_FLOAT1)
output(mask,'MP_OPACITY');output(color(ripple,.2,.26,.27),'MP_EMISSIVE_COLOR');ME.recompile_material(ripple);save(ripple)

physical={}
for name,kind,speed,foot in [('Soil',unreal.AltaiSurface.SOIL,1,None),('Stone',unreal.AltaiSurface.STONE,1,None),('Mud',unreal.AltaiSurface.MUD,.38,'M_FootprintMud'),('Water',unreal.AltaiSurface.WATER,.55,None),('Snow',unreal.AltaiSurface.SNOW,.78,'M_FootprintSnow'),('Wood',unreal.AltaiSurface.WOOD,1,None)]:
 p=unreal.load_asset(R+'/Surfaces/PM_'+name)
 if not p:
  f=unreal.PhysicalMaterialFactoryNew();f.set_editor_property('physical_material_class',unreal.AltaiPhysicalMaterial)
  p=AT.create_asset('PM_'+name,R+'/Surfaces',unreal.AltaiPhysicalMaterial,f)
 p.set_editor_property('kind',kind);p.set_editor_property('speed_multiplier',speed)
 if foot:p.set_editor_property('footprint',unreal.load_asset(R+'/Materials/'+foot))
 save(p);physical[name]=p
terrain.set_editor_property('phys_material',physical['Soil']);save(terrain)
for ident,mats in DATA['materials'].items():
 for path in mats.values():
  m=unreal.load_asset(path);m.set_editor_property('phys_material',physical['Wood' if 'trunk' in ident or 'tree' in ident else 'Stone']);save(m)

audio=unreal.AssetImportTask();audio.filename=str(Path(unreal.Paths.project_dir(),'SourceArt/Environment/Audio/Thunder_2718.wav'));audio.destination_path=R+'/Audio';audio.destination_name='Thunder_2718';audio.automated=True;audio.save=True;AT.import_asset_tasks([audio])

profiles=[]
for i,(name,rain,snow,mist,cloud,thunder) in enumerate([
 ('Clear',0,0,.1,.15,False),('Overcast',0,0,.28,.8,False),('Rain',.8,0,.42,.9,False),('Fog',0,0,.95,.7,False),('Thunderstorm',1,0,.65,1,True),('Snow',0,1,.5,.85,False),('Sleet',.45,.55,.7,1,False)]):
 path=R+'/Weather/DA_'+name;p=unreal.load_asset(path)
 if not p:
  f=unreal.DataAssetFactory();f.set_editor_property('data_asset_class',unreal.AltaiWeatherPreset);p=AT.create_asset('DA_'+name,R+'/Weather',unreal.AltaiWeatherPreset,f)
 for k,v in dict(label=name,rain=rain,snow=snow,mist=mist,cloud=cloud,thunder=thunder).items():p.set_editor_property(k,v)
 save(p);profiles.append(p)

# Preserve the user's original lab before the first natural upgrade.
backup='/Game/Altai/Debug/Maps/L_CharacterLab_Blockout'
if not EA.does_asset_exist(backup):
 if not EA.duplicate_asset(MAP,backup):raise RuntimeError('Cannot preserve original map')
 EA.save_asset(backup)
LEVEL.load_level(MAP)
existing=ACT.get_all_level_actors()
if any(a.get_actor_label()=='Altai_Valley_Landscape' for a in existing):
 raise RuntimeError('Natural lab already exists. Edit saved actors; do not rebuild over manual changes.')
for a in existing:
 if not isinstance(a,(unreal.WorldSettings,unreal.LevelScriptActor)):
  ACT.destroy_actor(a)

modepath='/Game/Altai/Debug/Framework/BP_NaturalLabMode'
mode=unreal.load_asset(modepath)
if not mode:
 f=unreal.BlueprintFactory();f.set_editor_property('parent_class',unreal.GameModeBase);mode=AT.create_asset('BP_NaturalLabMode','/Game/Altai/Debug/Framework',unreal.Blueprint,f)
c=unreal.get_default_object(unreal.load_class(None,modepath+'.BP_NaturalLabMode_C'))
c.set_editor_property('default_pawn_class',unreal.load_class(None,'/Game/Altai/Player/BP_AltaiCharacter.BP_AltaiCharacter_C'))
c.set_editor_property('player_controller_class',unreal.AltaiLabController);c.set_editor_property('hud_class',unreal.AltaiLabHUD);save(mode)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,modepath+'.BP_NaturalLabMode_C'))
world.get_world_settings().set_editor_property('kill_z',-1800)
raw=Path(unreal.Paths.project_dir(),'SourceArt/Environment/AltaiValley.r16')
raw.write_bytes(b''.join(struct.pack('<H',max(0,min(65535,round(32768+height(-7560+x*60,-7560+y*60)*1.28)))) for y in range(253) for x in range(253)))
land=unreal.AltaiEditorLibrary.import_lab_landscape(str(raw),terrain)
if not land:raise RuntimeError('Landscape import failed')
land.set_folder_path('01_Landscape')

def spawn(kind,name,pos,folder,rot=(0,0,0)):
 a=ACT.spawn_actor_from_class(kind,unreal.Vector(*pos),unreal.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]));a.set_actor_label(name);a.set_folder_path(folder);return a

def mesh_actor(path,name,pos,scale,folder,rot=(0,0,0),collision=True):
 a=spawn(unreal.StaticMeshActor,name,pos,folder,rot);m=a.static_mesh_component;m.set_static_mesh(unreal.load_asset(path));a.set_actor_scale3d(unreal.Vector(*scale));m.set_collision_profile_name('BlockAll' if collision else 'NoCollision');return a

def box(name,pos,size,folder,mat=None,rot=(0,0,0),hidden=False):
 a=mesh_actor('/Engine/BasicShapes/Cube',name,pos,tuple(v/100 for v in size),folder,rot)
 if mat:a.static_mesh_component.set_material(0,mat)
 if hidden:a.set_actor_hidden_in_game(True)
 return a

def scanned(ident,name,x,y,target_height,folder,yaw=0,pitch=0,z=None):
 choices=DATA['meshes'][ident]
 if not choices:raise RuntimeError('No meshes for '+ident)
 info=choices[RAND.randrange(len(choices))];h=info['max'][2]-info['min'][2];s=target_height/max(1,h)
 cx=(info['min'][0]+info['max'][0])/2;cy=(info['min'][1]+info['max'][1])/2
 # Mesh vertices can retain FBX offsets; transform desired bottom-centre explicitly.
 angle=math.radians(yaw);ox=(cx*math.cos(angle)-cy*math.sin(angle))*s;oy=(cx*math.sin(angle)+cy*math.cos(angle))*s
 zz=height(x,y) if z is None else z
 return mesh_actor(info['path'],name,(x-ox,y-oy,zz-info['min'][2]*s),(s,s,s),folder,(pitch,yaw,0),collision=not any(v in ident for v in ['tree','sapling','grass']))

rig=spawn(unreal.AltaiWeatherRig,'Weather_Director',(0,0,0),'00_Weather')
rig.set_editor_property('thunder_sound',unreal.load_asset(R+'/Audio/Thunder_2718'));rig.set_editor_property('presets',profiles);rig.set_editor_property('preset_index',1);rig.set_editor_property('surface_parameters',MPC);rig.set_editor_property('hour',15);rig.preview_weather()
spawn(unreal.PlayerStart,'Field_Start',(4700,-3100,height(4700,-3100)+115),'02_Trail',(0,145,0))

# A scan-built face over the sculptable mountain. Exposed rock shelves have their own collision.
for i,(x,y,h) in enumerate([(-4050,2500,2400),(-4400,1100,1800),(-3700,3700,2200),(-4950,3200,2600)]):
 scanned('rock_face_01','Cliff_Face_%02d'%i,x,y,h,'03_ClimbingRock',yaw=RAND.uniform(0,360),z=height(x,y)-450)
for i in range(7):
 x=-2900-i*145;y=350+i*180;z=height(x,y)
 scanned('rock_moss_set_01','Handhold_Ledge_%02d'%i,x,y,80+20*(i%3),'03_ClimbingRock',yaw=i*37,z=z-25)
 # Visible editor marker identifies the intended hand contact target, not an implemented grip.
 marker=spawn(unreal.TargetPoint,'GripTarget_%02d'%i,(x,y,z+70),'03_ClimbingRock/GripTargets')

# Irregular tree groups leave all test routes and wet areas open.
for i in range(85):
 x=RAND.uniform(-6300,6300);y=RAND.uniform(-6200,6200)
 if radius(x,y,1800,1700,1900,1450)<1 or radius(x,y,-700,1300,1350,1100)<1:continue
 if abs(y+3100)<350 or (x>2500 and -3500<y<1200) or (-3300<x<-1600 and -2900<y<-1500):continue
 if height(x,y)>1100:continue
 a=scanned('pine_sapling_small','Pine_%03d'%i,x,y,RAND.uniform(650,1100),'04_Forest',yaw=RAND.uniform(0,360))
 # Narrow trunk proxy keeps the canopy from blocking movement.
 box('TrunkCollision_%03d'%i,(x,y,height(x,y)+180),(38,38,360),'04_Forest/Collision',hidden=True)
for i in range(35):
 x=RAND.uniform(-4200,5200);y=RAND.uniform(-4600,4500)
 if radius(x,y,1800,1700,1700,1300)<1 or radius(x,y,-700,1300,1200,1000)<1 or abs(y+3100)<300:continue
 scanned('pine_sapling_small','Pine_Undergrowth_%03d'%i,x,y,RAND.uniform(120,260),'04_Forest/Undergrowth',yaw=RAND.uniform(0,360))
for i in range(65):
 x=RAND.uniform(-5500,5800);y=RAND.uniform(-5500,5500)
 if radius(x,y,1800,1700,1600,1200)<1:continue
 scanned('rock_moss_set_01','Moss_Boulder_%03d'%i,x,y,RAND.uniform(25,180),'05_Rocks',yaw=RAND.uniform(0,360),z=height(x,y)-15)
for i,(x,y) in enumerate([(3300,-2100),(3000,-1200),(3600,-200),(3800,700)]):
 scanned('dead_tree_trunk','Trail_FallenWood_%02d'%i,x,y,60,'02_Trail/Obstacles',yaw=35+i*50,z=height(x,y)-8)
 # A deliberate low collider gives the shin trace a stable test target.
 box('Low_Obstacle_%02d'%i,(x,y,height(x,y)+17),(240,28,34),'02_Trail/Obstacles',mat=unreal.load_asset(next(iter(DATA['materials']['dead_tree_trunk'].values()))),rot=(0,i*30,0))

# Oval shallow water mesh follows the natural basin boundary, not a rectangular plane.
obj=Path(unreal.Paths.project_dir(),'SourceArt/Environment/ShallowWater.obj')
count=64
points=[(0,0,0)]+[(math.cos(i/count*math.tau),math.sin(i/count*math.tau),0) for i in range(count)]
verts=['o ShallowWater']+['v %f %f %f'%v for v in points]+['vt %f %f'%((v[0]+1)/2,(v[1]+1)/2) for v in points]+['vn 0 0 1']
verts+=['f 1/1/1 %d/%d/1 %d/%d/1'%(i+2,i+2,(i+1)%count+2,(i+1)%count+2) for i in range(count)]
obj.write_text('\n'.join(verts))
# Use engine disk mesh from imported flat OBJ; import treats these vertices as centimetres.
t=unreal.AssetImportTask();t.filename=str(obj);t.destination_path=R+'/Geometry';t.destination_name='SM_ShallowWater';t.automated=True;t.save=True;AT.import_asset_tasks([t])
watermesh=next((unreal.load_asset(p) for p in t.imported_object_paths if isinstance(unreal.load_asset(p),unreal.StaticMesh)),None)
if not watermesh:raise RuntimeError('Water mesh import failed')
a=mesh_actor(watermesh.get_path_name(),'Shallow_Basin',(1800,1700,28),(1500,1100,1),'06_Wetlands',collision=False);a.static_mesh_component.set_material(0,water)
for name,x,y,rx,ry,top,kind,depth in [('Brod',1800,1700,1500,1100,28,'Water',60),('Bog',-700,1300,1050,850,55,'Mud',28),('SnowPrints',-2600,-2200,700,500,500,'Snow',1)]:
 z=spawn(unreal.AltaiSurfaceZone,'Surface_'+name,(x,y,top-100),'06_Wetlands' if kind!='Snow' else '07_Snow')
 z.bounds.set_box_extent(unreal.Vector(rx,ry,600),False);z.set_editor_property('surface',physical[kind]);z.set_editor_property('surface_height',top);z.set_editor_property('full_resistance_depth',depth)

# Reed/grass islands emphasize the shallows and retain a readable walking route.
for i in range(140):
 a=RAND.uniform(0,math.tau);r=RAND.uniform(.95,1.25);x=1800+math.cos(a)*1500*r;y=1700+math.sin(a)*1100*r
 scanned('grass_bermuda_01','Bankside_Grass_%03d'%i,x,y,RAND.uniform(30,65),'06_Wetlands/Grass',yaw=RAND.uniform(0,360))

# Tactile testing camp and sheltered comparison point.
wood=unreal.load_asset(next(iter(DATA['materials']['dead_tree_trunk'].values())))
for i,(x,y) in enumerate([(4100,-3400),(4700,-3400),(4100,-3900),(4700,-3900)]):box('Shelter_Post_%d'%i,(x,y,height(x,y)+150),(24,24,300),'08_Camp',wood)
box('Shelter_Roof',(4400,-3650,height(4400,-3650)+320),(720,650,35),'08_Camp',wood,rot=(0,0,8))
box('Worktable',(4300,-3550,height(4300,-3550)+85),(90,250,15),'08_Camp',wood)
for i,mass in enumerate([1,5,20]):
 a=mesh_actor('/Engine/BasicShapes/Sphere','Loose_Stone_%dkg'%mass,(4280,-3640+i*85,height(4300,-3550)+120),(.28,.3,.22),'08_Camp/Physics')
 a.static_mesh_component.set_material(0,unreal.load_asset(next(iter(DATA['materials']['rock_moss_set_01'].values()))));a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE);a.static_mesh_component.set_collision_profile_name('PhysicsActor');a.static_mesh_component.set_mass_override_in_kg(unreal.Name('None'),mass,True);a.static_mesh_component.set_simulate_physics(True)

def sign(name,text,x,y,yaw=0):
 z=height(x,y)
 box(name+'_Post',(x,y,z+100),(10,10,200),'09_Wayfinding',wood)
 board=box(name+'_Board',(x,y,z+180),(8,260,90),'09_Wayfinding',wood,rot=(0,yaw,0))
 a=spawn(unreal.TextRenderActor,name,(x+8*math.cos(math.radians(yaw)),y+8*math.sin(math.radians(yaw)),z+205),'09_Wayfinding',(0,yaw,0))
 c=a.get_component_by_class(unreal.TextRenderComponent);c.set_text(text);c.set_world_size(18);c.set_text_render_color(unreal.Color(205,198,165,255))
for args in [('CampSign','ALTAI / FIELD LAB\nH - CONTROLS\nFOLLOW THE STONE TRAIL',4800,-2800,0),('RockSign','01 / ROCK FACE\nHAND CONTACT TARGETS',-2300,200,0),('MudSign','02 / SOFT GROUND\nRESISTANCE + FOOTPRINTS',400,700,0),('WaterSign','03 / SHALLOW WATER\nDEPTH + WET BOOTS',3300,700,0),('SnowSign','04 / SNOW\nFOOTPRINT STUDY',-1600,-2400,0)]:sign(*args)

unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(5800,-4400,620),unreal.Rotator(pitch=-5,yaw=135,roll=0))
if not LEVEL.save_current_level():raise RuntimeError('Cannot save natural lab')
Path(unreal.Paths.project_saved_dir(),'natural_lab_created.json').write_text(json.dumps({'map':MAP,'actors':len(ACT.get_all_level_actors()),'landscape':land.get_path_name(),'backup':backup},indent=2))
unreal.log('ALTAI_NATURAL_LAB_CREATED')
