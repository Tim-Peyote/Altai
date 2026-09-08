"""Match the visible cliff lips to the tested collision and keep landing capsules clear."""
import unreal,sys,importlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent));import lab_test_geometry;importlib.reload(lab_test_geometry)
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors={a.get_actor_label():a for a in A.get_all_level_actors()};R='/Game/Altai/Environment'
folder=Path(unreal.Paths.project_dir(),'SourceArt/Environment/FieldTests');lab_test_geometry.make_all(folder)
t=unreal.AssetImportTask();t.filename=str(folder/'SM_ClimbingEscarpment.obj');t.destination_path=R+'/Geometry';t.destination_name='SM_ClimbingEscarpment_Traversable';t.replace_existing=True;t.automated=True;t.save=True;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
actors['Climbing_Escarpment'].static_mesh_component.set_static_mesh(unreal.load_asset(R+'/Geometry/SM_ClimbingEscarpment_Traversable'));actors['Climbing_Escarpment'].static_mesh_component.set_material(0,unreal.load_asset(R+'/Materials/M_LabCliff'))
for label in ['Rough','Smooth','Moss']:
 wall=actors['Climbing_Test_'+label];center,ext=wall.get_actor_bounds(False);top=center.z+ext.z;name='Climbing_Landing_'+label
 a=actors.get(name) or A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(3350,center.y,top-30));a.set_actor_label(name);a.set_folder_path('03_ClimbingRock/Collision');a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));a.set_actor_location(unreal.Vector(3350,center.y,top-30),False,True);a.set_actor_scale3d(unreal.Vector(4,9,.6));a.static_mesh_component.set_collision_profile_name('BlockAll');a.set_actor_hidden_in_game(True)
 # A small route mark is useful for testing lip acquisition from first person.
 name='Climbing_Exit_'+label;mark=actors.get(name) or A.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(3570,center.y+200,top-55));mark.set_actor_label(name);mark.set_folder_path('03_ClimbingRock/RouteMarks');mark.set_actor_rotation(unreal.Rotator(0,0,0),False);mark.text_render.set_text('E / PULL UP');mark.text_render.set_world_size(10);mark.text_render.set_text_render_color(unreal.Color(210,197,155,255))
# The backing must not protrude through the lower route's landing.
back=actors['Climbing_BackCollision'];back.set_actor_location(unreal.Vector(3225,-4300,650),False,True);back.set_actor_scale3d(unreal.Vector(6.4,33,15))
for a in actors.values():
 if a.get_actor_label().startswith('Climbing_Crown_'):
  center,ext=a.get_actor_bounds(False);p=a.get_actor_location();p.x-=max(0,center.x+ext.x-3110);a.set_actor_location(p,False,True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('CLIMB_LANDINGS_FITTED')
