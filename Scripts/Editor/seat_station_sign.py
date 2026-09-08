import unreal,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_landform import height
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors={a.get_actor_label():a for a in A.get_all_level_actors()};wood=unreal.load_asset('/Game/Altai/Environment/Materials/M_FieldWood')
t=actors['Carry_Station_Header'];t.set_actor_location(unreal.Vector(4990,-3150,205),False,True);t.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False);t.text_render.set_text('02 / HANDS + LOAD\nF TAKE / RELEASE - V VIEW\nB BODY MASS');t.text_render.set_world_size(12)
for name,xyz,size in [('Carry_Station_Board',(5000,-3000,180),(8,360,90)),('Carry_Station_Post',(5000,-3000,(height(5000,-3000)+175)/2),(10,10,175-height(5000,-3000)))]:
 a=actors.get(name) or A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*xyz));a.set_actor_label(name);a.set_actor_location(unreal.Vector(*xyz),False,True);a.set_folder_path('09_Wayfinding');a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));a.static_mesh_component.set_material(0,wood);a.static_mesh_component.set_collision_profile_name('BlockAll');a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('STATION_SIGN_SEATED')
