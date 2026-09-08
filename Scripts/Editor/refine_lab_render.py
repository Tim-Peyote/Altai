import unreal,json
from pathlib import Path
EA=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();ACT=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);R='/Game/Altai/Environment'
# Screen-exposure compensated particles remain legible with a physical daylight exposure.
m=unreal.load_asset(R+'/Materials/M_WeatherParticle')
if not m:
 m=AT.create_asset('M_WeatherParticle',R+'/Materials',unreal.Material,unreal.MaterialFactoryNew());m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT);m.set_editor_property('two_sided',True)
 def node(cls):return ME.create_material_expression(m,cls)
 pc=node(unreal.MaterialExpressionParticleColor);eye=node(unreal.MaterialExpressionEyeAdaptation);div=node(unreal.MaterialExpressionDivide)
 ME.connect_material_expressions(pc,'RGB',div,'A');ME.connect_material_expressions(eye,'',div,'B');ME.connect_material_property(div,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 uv=node(unreal.MaterialExpressionTextureCoordinate);shape=node(unreal.MaterialExpressionCustom);shape.set_editor_property('code','float2 d=(UV-.5)*2;return saturate(1-dot(d,d))*Alpha;');shape.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
 ins=[]
 for name in ['UV','Alpha']:
  i=unreal.CustomInput();i.set_editor_property('input_name',name);ins.append(i)
 shape.set_editor_property('inputs',ins);ME.connect_material_expressions(uv,'',shape,'UV');ME.connect_material_expressions(pc,'A',shape,'Alpha');ME.connect_material_property(shape,'',unreal.MaterialProperty.MP_OPACITY)
 ME.recompile_material(m);EA.save_loaded_asset(m)
report={}
for a in ACT.get_all_level_actors():
 if a.get_actor_label()=='Distant_Altai_Ridges':
  report['ridge_bounds']=str(a.static_mesh_component.static_mesh.get_bounding_box());report['ridge_transform']=str(a.get_actor_transform())
  # OBJ faces from source use the opposite winding convention to Unreal. Render both sides on the background only.
  mat=unreal.load_asset(R+'/Materials/M_DistantRidges')
  if not mat:mat=EA.duplicate_asset(R+'/Materials/M_ValleyLandscape',R+'/Materials/M_DistantRidges')
  mat.set_editor_property('two_sided',True);ME.recompile_material(mat);EA.save_loaded_asset(mat);a.static_mesh_component.set_material(0,mat)
 if a.get_actor_label()=='Shallow_Basin':
  a.set_actor_rotation(unreal.Rotator(0,0,0),False);a.set_actor_scale3d(unreal.Vector(1500,1100,1));mat=unreal.load_asset(R+'/Materials/M_ShallowWater');mat.set_editor_property('two_sided',True);ME.recompile_material(mat);EA.save_loaded_asset(mat);a.static_mesh_component.set_material(0,mat)
 if isinstance(a,unreal.AltaiWeatherRig):
  report['hidden']=a.get_editor_property('hidden');report['scale']=str(a.get_actor_scale3d())
  for n in ['fog','clouds','atmosphere']:
   c=a.get_editor_property(n);report[n]={'hidden':c.get_editor_property('hidden_in_game'),'registered':c.is_registered() if hasattr(c,'is_registered') else 'na'}
  a.fog.set_fog_inscattering_color(unreal.LinearColor(1100,1400,1450,1));a.fog.set_fog_density(.08)
  a.clouds.set_layer_bottom_altitude(.6);a.clouds.set_layer_height(2)
# Keep the completed scene and authoring report.
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();Path(unreal.Paths.project_saved_dir(),'lab_render_diagnostic.json').write_text(json.dumps(report,indent=2))
unreal.log('LAB_RENDER_REFINED')
