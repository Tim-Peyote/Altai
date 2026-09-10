"""Distance absorption for the camera-underwater post process; no screen distortion."""
import unreal
E=unreal.EditorAssetLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();M=unreal.MaterialEditingLibrary
folder='/Game/Altai/Environment/Materials';name='M_UnderwaterDepth'
a=unreal.load_asset(folder+'/'+name) or T.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
M.delete_all_material_expressions(a);a.set_editor_property('material_domain',unreal.MaterialDomain.MD_POST_PROCESS)
color=M.create_material_expression(a,unreal.MaterialExpressionSceneTexture,0,0);color.set_editor_property('scene_texture_id',unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
z=M.create_material_expression(a,unreal.MaterialExpressionSceneDepth,0,160)
c=M.create_material_expression(a,unreal.MaterialExpressionCustom,200,0);c.set_editor_property('code','return lerp(C.rgb,float3(0.018,0.075,0.062),1-exp(-max(D,0)/800.0));');c.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs=[]
for name in ['C','D']:
 i=unreal.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
c.set_editor_property('inputs',inputs)
M.connect_material_expressions(color,'Color',c,'C');M.connect_material_expressions(z,'',c,'D');M.connect_material_property(c,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);M.recompile_material(a)
if not E.save_loaded_asset(a,False):raise RuntimeError('Could not save underwater material')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors();pond=next(a for a in actors if isinstance(a,unreal.AltaiPond))
water=unreal.load_asset(folder+'/PM_PondWater') or E.duplicate_asset(pond.surface.get_path_name(),folder+'/PM_PondWater')
water.set_editor_property('speed_multiplier',.28);E.save_loaded_asset(water,False);pond.surface=water
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
