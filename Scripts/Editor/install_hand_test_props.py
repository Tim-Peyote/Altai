"""Add light grip/rotation props and a short throwing lane to the existing camp."""
import unreal,sys,shutil,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from make_hand_test_props import make
from lab_landform import height
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert 'L_CharacterLab' in world.get_path_name()
root=Path(unreal.Paths.project_dir());src=root/'SourceArt/Environment/HandTests';make(src)
backup=root/'Saved/Backups/ObjectManipulation/L_CharacterLab.umap';backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists():shutil.copy2(root/'Content/Altai/Debug/Maps/L_CharacterLab.umap',backup.with_suffix('.umap'))
E=unreal.EditorAssetLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();SM=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem);A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
R='/Game/Altai/Environment';existing={a.get_actor_label():a for a in A.get_all_level_actors()}
def save(o):E.save_loaded_asset(o,False)
def material(name,color,roughness):
 p=R+'/Materials/'+name
 if E.does_asset_exist(p):return unreal.load_asset(p)
 m=AT.create_asset(name,R+'/Materials',unreal.Material,unreal.MaterialFactoryNew());ME=unreal.MaterialEditingLibrary
 c=ME.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);c.constant=unreal.LinearColor(*color,1);ME.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
 r=ME.create_material_expression(m,unreal.MaterialExpressionConstant);r.r=roughness;ME.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS);ME.recompile_material(m);save(m);return m
ceramic=material('M_HandFlaskGlaze',(.065,.13,.09),.4);bottle=material('M_HandBottleGlaze',(.16,.09,.045),.42)
steel=unreal.load_asset(R+'/Materials/M_FieldSteel');wood=unreal.load_asset(R+'/Materials/M_FieldWood')
meshes={}
for name,mat in [('SM_HandFlask',ceramic),('SM_HandBottle',bottle),('SM_HandCup',steel)]:
 path=R+'/Geometry/'+name
 if not E.does_asset_exist(path):
  t=unreal.AssetImportTask();t.filename=str(src/(name+'.obj'));t.destination_path=R+'/Geometry';t.destination_name=name;t.automated=True;t.save=True
  opt=unreal.FbxImportUI();opt.import_materials=False;opt.import_textures=False;opt.static_mesh_import_data.combine_meshes=True;t.options=opt;AT.import_asset_tasks([t])
 m=unreal.load_asset(path);assert m,name;m.set_material(0,mat)
 SM.remove_collisions(m);SM.add_simple_collisions(m,unreal.ScriptingCollisionShapeType.NDOP26)
 m.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
 if name.endswith('Cup'):
  SM.remove_collisions(m);boxes=[]
  def shape(center,size,yaw=0):
   b=unreal.KBoxElem();b.set_editor_property('center',unreal.Vector(*center));b.set_editor_property('rotation',unreal.Rotator(pitch=0,yaw=yaw,roll=0))
   for axis,value in zip(['x','y','z'],size):b.set_editor_property(axis,value)
   boxes.append(b)
  for i in range(16):
   a=i*math.tau/16;shape((5*math.cos(a),5*math.sin(a),5),(2.2,.8,10),i*360/16+90)
  shape((0,0,.6),(8,8,1.2));shape((0,8,8.5),(1.3,5,1.3));shape((0,9.5,5.5),(1.3,1.3,6));shape((0,8,2.5),(1.3,5,1.3))
  body=m.get_editor_property('body_setup');agg=body.get_editor_property('agg_geom');agg.set_editor_property('box_elems',boxes);body.set_editor_property('agg_geom',agg)
 def socket(n,pos,finger,palm):
  s=m.find_socket(n)
  if not s:s=unreal.new_object(unreal.StaticMeshSocket,outer=m);s.set_editor_property('socket_name',n);m.add_socket(s)
  s.set_editor_property('relative_location',unreal.Vector(*pos));s.set_editor_property('relative_rotation',unreal.MathLibrary.make_rot_from_xz(unreal.Vector(*finger),unreal.Vector(*palm)))
 radius=5 if name.endswith('Bottle') else 3.7
 z=6 if name.endswith('Bottle') else (5 if name.endswith('Cup') else 3.5)
 socket('Grip_One',(0,11 if name.endswith('Cup') else radius+4,z),(0,0,1),(0,-1,0))
 socket('Grip_L',(0,-radius-4,z),(0,0,1),(0,1,0));socket('Grip_R',(0,radius+4,z+3),(0,0,1),(0,-1,0));save(m);meshes[name]=m
# The loaded bucket is supported at its rim; the old wrists at z=23 cut thumbs through the wall.
bucket_mesh=unreal.load_asset(R+'/Geometry/SM_FieldBucket')
for name in ['Grip_L','Grip_R']:
 grip=bucket_mesh.find_socket(name);pos=grip.get_editor_property('relative_location');pos.z=34;grip.set_editor_property('relative_location',pos)
save(bucket_mesh)
pm_path=R+'/Materials/PM_HandTestProp';pm=unreal.load_asset(pm_path)
if not pm:pm=AT.create_asset('PM_HandTestProp',R+'/Materials',unreal.PhysicalMaterial,unreal.PhysicalMaterialFactoryNew())
pm.set_editor_property('friction',.6);pm.set_editor_property('restitution',.12);save(pm)
def spawn(cls,name,pos):
 a=existing.get(name) or A.spawn_actor_from_class(cls,unreal.Vector(*pos),unreal.Rotator());a.set_actor_label(name);a.set_folder_path('08_Camp/ThrowTests');a.set_actor_location(unreal.Vector(*pos),False,True);existing[name]=a;return a
def box(name,pos,size,mass=None):
 a=spawn(unreal.StaticMeshActor,name,pos);c=a.static_mesh_component;c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));c.set_material(0,wood);a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)));c.set_collision_profile_name('PhysicsActor' if mass else 'BlockAll')
 if mass:c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_mass_override_in_kg('None',mass,True);c.set_simulate_physics(True)
 return a
def label(name,words,pos,size=8):
 a=spawn(unreal.TextRenderActor,name,pos);a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False);a.text_render.set_text(words);a.text_render.set_world_size(size);a.text_render.set_text_render_color(unreal.Color(222,211,172,255));return a
x,y=4650,-3200;floor=height(x,y)+4;top=floor+82
box('Throw_Test_Table',(x,y,top-5),(90,260,10))
for i,(dx,dy) in enumerate([(-34,-112),(34,-112),(-34,112),(34,112)]):box('Throw_Table_Leg_'+str(i),(x+dx,y+dy,(floor+top)/2),(10,10,top-floor))
for name,geo,mass,dy,mat in [('Throw_Flask_025kg','SM_HandFlask',.25,-80,ceramic),('Throw_Bottle_1kg','SM_HandBottle',1,0,bottle),('Throw_Cup_04kg','SM_HandCup',.4,80,steel)]:
 a=spawn(unreal.StaticMeshActor,name,(x,y+dy,top+2));c=a.static_mesh_component;c.set_static_mesh(meshes[geo]);c.set_material(0,mat);c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_collision_profile_name('PhysicsActor');c.set_mass_override_in_kg('None',mass,True);c.set_phys_material_override(pm);c.set_simulate_physics(True)
 label(name+'_Label',name.replace('Throw_','').replace('_',' '),(x+46,y+dy-35,top-3),6)
label('Throw_Test_Header','THROW + ROTATE\nLMB HOLD / SWING / RELEASE\nRMB CHARGE / RELEASE - R ROTATE\nHAND LIMITS - WHEEL DEPTH / FOREARM - T GRIP',(x+120,y-160,top+140),10)
# Targets stand beyond the table, away from the furniture and balance-beam approaches.
for i,mass in enumerate([1,3,6]):
 tx,ty=5200,-3100+i*85;z=height(tx,ty)
 box('Throw_Target_Stand_'+str(i),(tx,ty,z+30),(50,60,60));box('Throw_Target_'+str(mass)+'kg',(tx,ty,z+78),(12,32,36),mass)
label('Throw_Target_Label','THROW LANE / 1 - 3 - 6 KG TARGETS',(5190,-3200,height(5190,-3200)+135),9).set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
import runpy
runpy.run_path(str(Path(__file__).with_name('author_grasp_profiles.py')))
unreal.log('ALTAI_HAND_TEST_PROPS_INSTALLED')
