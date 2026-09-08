import unreal
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors={a.get_actor_label():a for a in A.get_all_level_actors()};R='/Game/Altai/Environment'
for i,y in enumerate([-3500,-3830]):
 name='Camp_TaskLight_'+str(i);a=actors.get(name) or A.spawn_actor_from_class(unreal.PointLight,unreal.Vector(4440,y,280));a.set_actor_label(name);a.set_folder_path('08_Camp/Lighting');c=a.point_light_component;c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_intensity(350);c.set_attenuation_radius(400);c.set_light_color(unreal.LinearColor(1,.76,.48,1));c.set_cast_shadows(False)
parent=actors['Carry_Bucket_Ballast_12kg'];p=parent.get_actor_location()
for i,(x,y,z) in enumerate([(-6,-5,24),(6,-4,24),(0,7,26)]):
 name='Bucket_Ballast_Visual_'+str(i);a=actors.get(name) or A.spawn_actor_from_class(unreal.StaticMeshActor,p+unreal.Vector(x,y,z));a.set_actor_label(name);a.set_folder_path('08_Camp/Physics/Ballast');c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset(R+'/Geometry/SM_FieldStone'));c.set_material(0,unreal.load_asset(R+'/Materials/M_GripStone'));c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_collision_profile_name('NoCollision');a.set_actor_scale3d(unreal.Vector(.48,.48,.55));a.attach_to_actor(parent,unreal.Name('None'),unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,False)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('CAMP_LIGHT_AND_BALLAST_SAVED')
