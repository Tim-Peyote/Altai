"""Import licensed separate furniture parts and assemble a physical practice alcove."""
import unreal,json
from pathlib import Path
ROOT='/Game/Altai/Environment/Interaction';SRC=Path(unreal.Paths.project_dir())/'SourceArt/Environment/InteractionAssets';manifest=json.loads((SRC/'Prepared/manifest.json').read_text())
AT=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary;SM=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem);A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
def save(o):E.save_loaded_asset(o,only_if_is_dirty=False)
def import_file(file,name):
 p=ROOT+'/'+name
 if not E.does_asset_exist(p):
  t=unreal.AssetImportTask();t.filename=str(file);t.destination_path=ROOT;t.destination_name=name;t.automated=True;t.save=True
  if file.suffix=='.obj':
   opt=unreal.FbxImportUI();opt.import_materials=False;opt.import_textures=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=False;t.options=opt
  AT.import_asset_tasks([t])
 obj=unreal.load_asset(p)
 if not obj:raise RuntimeError('Import failed '+p)
 return obj
materials={}
for ident in sorted({e['source'] for e in manifest}):
 tex={}
 for kind in ['diff','arm','nor_gl']:
  file=next((SRC/ident/'textures').glob('*_'+kind+'_2k.*'));t=import_file(file,'T_PH_'+ident+'_'+kind)
  if kind!='diff':t.set_editor_property('srgb',False)
  if kind=='nor_gl':t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
  t.set_editor_property('max_texture_size',2048);save(t);tex[kind]=t
 name='M_PH_'+ident;m=unreal.load_asset(ROOT+'/'+name) or AT.create_asset(name,ROOT,unreal.Material,unreal.MaterialFactoryNew());ME.delete_all_material_expressions(m)
 for kind in tex:
  n=ME.create_material_expression(m,unreal.MaterialExpressionTextureSample);n.texture=tex[kind]
  if kind=='diff':ME.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
  elif kind=='nor_gl':n.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL;ME.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_NORMAL)
  else:
   n.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
   for channel,prop in [('R','MP_AMBIENT_OCCLUSION'),('G','MP_ROUGHNESS'),('B','MP_METALLIC')]:ME.connect_material_property(n,channel,getattr(unreal.MaterialProperty,prop))
 ME.recompile_material(m);save(m);materials[ident]=m
meshes={}
for e in manifest:
 name=e['name'];m=import_file(SRC/'Prepared'/(name+'.obj'),name);m.set_material(0,materials[e['source']]);body=m.get_editor_property('body_setup')
 SM.remove_collisions(m)
 if 'axis' not in e:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
 else:
  body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
  if e.get('sliding'):
   # Open box: four walls and a bottom; contents never sit on an invisible top hull.
   (x0,x1),(y0,y1),(z0,z1)=e['bounds'];th=1.5;boxes=[]
   dims=[((x0+x1)/2,(y0+y1)/2,z0+th/2,x1-x0,y1-y0,th),((x0+x1)/2,y0+th/2,(z0+z1)/2,x1-x0,th,z1-z0),((x0+x1)/2,y1-th/2,(z0+z1)/2,x1-x0,th,z1-z0),(x0+th/2,(y0+y1)/2,(z0+z1)/2,th,y1-y0,z1-z0),(x1-th/2,(y0+y1)/2,(z0+z1)/2,th,y1-y0,z1-z0)]
   for x,y,z,dx,dy,dz in dims:
    b=unreal.KBoxElem();b.set_editor_property('center',unreal.Vector(x,y,z));b.set_editor_property('x',dx);b.set_editor_property('y',dy);b.set_editor_property('z',dz);boxes.append(b)
   agg=body.get_editor_property('agg_geom');agg.set_editor_property('box_elems',boxes);body.set_editor_property('agg_geom',agg)
  else:SM.add_simple_collisions(m,unreal.ScriptingCollisionShapeType.NDOP26)
  s=m.find_socket('Grip_One')
  if not s:s=unreal.new_object(unreal.StaticMeshSocket,outer=m);s.set_editor_property('socket_name','Grip_One');m.add_socket(s)
  s.set_editor_property('relative_location',unreal.Vector(*e['grip']));s.set_editor_property('relative_rotation',unreal.MathLibrary.make_rot_from_xz(unreal.Vector(0,0,-1),unreal.Vector(-1,0,0)))
 save(m);meshes[name]=m
existing={a.get_actor_label():a for a in A.get_all_level_actors()}
def spawn(cls,name,pos):
 a=existing.get(name) or A.spawn_actor_from_class(cls,unreal.Vector(*pos),unreal.Rotator());a.set_actor_label(name);a.set_folder_path('11_HandInteraction');a.set_actor_location(unreal.Vector(*pos),False,True);existing[name]=a;return a
wood=unreal.load_asset('/Game/Altai/Environment/Materials/M_FieldWood')
def box(name,pos,size):
 a=spawn(unreal.StaticMeshActor,name,pos);a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));a.static_mesh_component.set_material(0,wood);a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)));return a
def static(name,geo,pos):
 a=spawn(unreal.StaticMeshActor,name,pos);a.static_mesh_component.set_static_mesh(meshes[geo]);return a
def prop(name,geo,pos,base=None):
 a=spawn(unreal.AltaiArticulatedProp,name,pos);e=next(e for e in manifest if e['name']==geo)
 a.part.set_static_mesh(meshes[geo]);a.part.set_relative_location(unreal.Vector(*e['pivot']),False,True)
 if base:a.base.set_static_mesh(meshes[base])
 a.set_editor_property('axis',unreal.Vector(*e['axis']));a.set_editor_property('travel',e['travel']);a.set_editor_property('part_mass',e['mass']);a.set_editor_property('sliding',e.get('sliding',False));a.set_editor_property('resistance',4 if e.get('sliding') else 2.5)
 return a
def sign(name,words,pos,size=10):
 a=spawn(unreal.TextRenderActor,name,pos);a.text_render.set_text(words);a.text_render.set_world_size(size);a.text_render.set_text_render_color(unreal.Color(222,204,165,255));return a
# Level deck, separated from the beam and the climbing approach. Low approach steps.
box('Interaction_Deck',(4850,-5480,139),(750,1040,18))
for i in range(3):box('Interaction_Approach_'+str(i),(5260+i*55,-5220,115-i*15),(55,260,30))
# Perimeter uprights and back screen ground the large door in a small camp workshop.
for i,y in enumerate([-5000,-5380,-5890]):box('Interaction_Post_'+str(i),(4700,y,280),(18,18,264))
box('Interaction_BackBeam',(4700,-5445,406),(22,910,20))
# Clear frame, two individually hinged leaves. No wall blocks the opening.
prop('Interact_Door_Left','SM_PH_DoorLeft',(4780,-5120,148),'SM_PH_DoorFrame')
prop('Interact_Door_Right','SM_PH_DoorRight',(4780,-5120,148)).set_editor_property('anchor_actor',existing['Interact_Door_Left'])
# Raise the 82cm cabinet by 30cm: upper handles standing, lower handles crouched.
box('Interaction_CabinetStand',(4780,-5480,170.5),(86,148,45))
static('Interaction_CabinetBody','SM_PH_Cabinet',(4780,-5480,193))
for i in range(1,7):prop('Interact_Drawer_'+str(i),'SM_PH_Drawer'+str(i),(4780,-5480,193)).set_editor_property('anchor_actor',existing['Interaction_CabinetBody'])
box('Interaction_ChestStand',(4780,-5740,178),(75,112,60))
prop('Interact_Chest','SM_PH_ChestLid',(4780,-5740,208),'SM_PH_ChestBase')
sign('Interaction_Header','04 / HAND INTERACTION\nHOLD LMB + DRAG DOWN / UP\nF TOGGLE GRIP - CTRL CROUCH - V VIEW',(5150,-5680,335),13)
sign('Interaction_DoorLabel','HINGED DOOR / 24 + 30 KG',(4860,-5200,157),8)
sign('Interaction_DrawerLabel','6 INDEPENDENT DRAWERS / 3.6 - 6.6 KG',(4850,-5550,170),7)
sign('Interaction_ChestLabel','CHEST / 9 KG LID',(4820,-5790,192),8)
# Warm task light; atmosphere/weather remain controlled by the existing lab rig.
a=spawn(unreal.PointLight,'Interaction_WorkLight',(4900,-5500,410));a.point_light_component.set_intensity(2500);a.point_light_component.set_attenuation_radius(650);a.point_light_component.set_light_color(unreal.LinearColor(1,.72,.43))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
Path(unreal.Paths.project_saved_dir(),'interaction_furniture_installed.json').write_text(json.dumps({'actors':[n for n in existing if n.startswith(('Interact_','Interaction_'))],'assets':manifest},indent=2))
unreal.log('INTERACTION_FURNITURE_INSTALLED')

# Apply the finalized collision and clear approach geometry on initial install/rebuild.
import runpy
runpy.run_path(str(Path(__file__).with_name('clear_furniture_deck.py')))
runpy.run_path(str(Path(__file__).with_name('refine_furniture_colliders.py')))
