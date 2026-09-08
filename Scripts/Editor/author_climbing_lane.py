import unreal,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_landform import height
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
data=json.loads(Path(unreal.Paths.project_saved_dir(),'environment_assets.json').read_text())
material=unreal.load_asset(next(iter(data['materials']['rock_face_01'].values())))
existing={a.get_actor_label() for a in A.get_all_level_actors()}
for i,(label,tag,y) in enumerate([('Rough','RoughRock',-3100),('Smooth','SmoothRock',-4300),('Moss','MossyRock',-5500)]):
 name='Climbing_Test_'+label
 if name in existing:continue
 a=A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(3500,y,height(3500,y)+250))
 a.set_actor_label(name);a.set_folder_path('03_ClimbingRock/ContactTests')
 a.set_editor_property('tags',[unreal.Name('AltaiClimbable'),unreal.Name(tag)])
 c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
 c.set_material(0,material);c.set_collision_profile_name('BlockAll')
 a.set_actor_scale3d(unreal.Vector(1.2,6,5))
 t=A.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(3570,y-220,height(3500,y)+280),unreal.Rotator(pitch=0,yaw=0,roll=0))
 t.set_actor_label(name+'_Sign');t.set_folder_path('03_ClimbingRock/ContactTests')
 t.text_render.set_text(label.upper()+' ROCK\nE: ATTACH / C: RELEASE\nWASD: MOVE / RMB: LIMB / LMB: PLACE')
 t.text_render.set_world_size(12)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('CLIMBING_LANE_AUTHORED')
