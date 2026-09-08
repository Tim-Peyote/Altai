import unreal,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent));from lab_landform import height
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors={a.get_actor_label():a for a in A.get_all_level_actors()}
for name,target in [('Pine_049',(5420,-6120)),('Pine_060',(5530,-5610))]:
 a=actors[name];old=a.get_actor_location();x,y=target;new=unreal.Vector(x,y,height(x,y));delta=new-old;a.set_actor_location(new,False,True)
 trunk=actors.get(name.replace('Pine_','TrunkCollision_'))
 if trunk:trunk.set_actor_location(trunk.get_actor_location()+delta,False,True)
# Per-board UVs avoid the large stretched single-box platform.
old=actors.get('Interaction_Deck')
if old:A.destroy_actor(old)
wood=unreal.load_asset('/Game/Altai/Environment/Materials/M_FieldWood')
for i in range(30):
 name='Interaction_DeckBoard_'+str(i);a=actors.get(name) or A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(4487.5+i*25,-5480,139),unreal.Rotator());a.set_actor_label(name);a.set_folder_path('11_HandInteraction/Deck');a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));a.static_mesh_component.set_material(0,wood);a.set_actor_scale3d(unreal.Vector(.248,10.4,.18))
for a in actors.values():
 if isinstance(a,unreal.AltaiArticulatedProp) and not a.base.is_visible():a.base.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
