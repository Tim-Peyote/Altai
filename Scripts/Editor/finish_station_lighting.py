import unreal
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);E=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();R='/Game/Altai/Environment/Materials';actors={a.get_actor_label():a for a in A.get_all_level_actors()}
# Props retain their own roughness; rain still drives Wetness on their materials.
for name in ['M_FieldSteel','M_FieldWood']:
 m=unreal.load_asset(R+'/'+name)
 for n in (e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer()==m):
  if isinstance(n,unreal.MaterialExpressionScalarParameter) and str(n.get_editor_property('parameter_name'))=='DryRoughness':n.set_editor_property('parameter_name','PropRoughness')
 ME.recompile_material(m);E.save_loaded_asset(m)
name='M_LanternGlass';m=unreal.load_asset(R+'/'+name) or AT.create_asset(name,R,unreal.Material,unreal.MaterialFactoryNew())
ME.delete_all_material_expressions(m);n=ME.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);n.set_editor_property('constant',unreal.LinearColor(6,2.6,.7,1));ME.connect_material_property(n,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);ME.recompile_material(m);E.save_loaded_asset(m)
steel=unreal.load_asset(R+'/M_FieldSteel')
for i,y in enumerate([-3500,-3830]):
 light=actors['Camp_TaskLight_'+str(i)].point_light_component;light.set_editor_property('intensity_units',unreal.LightUnits.LUMENS);light.set_intensity(600)
 for part,z,scale,mat in [('Glass',280,(.07,.07,.14),m),('Top',288,(.1,.1,.025),steel),('Base',272,(.1,.1,.025),steel),('Hanger',326,(.008,.008,.7),steel)]:
  label='Camp_Lantern_'+str(i)+'_'+part;a=actors.get(label) or A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(4440,y,z));a.set_actor_label(label);a.set_folder_path('08_Camp/Lighting');a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cylinder'));a.static_mesh_component.set_material(0,mat);a.static_mesh_component.set_collision_profile_name('NoCollision');a.set_actor_scale3d(unreal.Vector(*scale))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('STATION_LIGHTING_FINISHED')
