import unreal,json
from pathlib import Path
E=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);R='/Game/Altai/Environment/Materials'
p=R+'/M_LabCliffPatch';m=unreal.load_asset(p) or E.duplicate_asset(R+'/M_LabCliff',p)
m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.333)
uv=ME.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate);pos=ME.create_material_expression(m,unreal.MaterialExpressionWorldPosition)
n=ME.create_material_expression(m,unreal.MaterialExpressionCustom);n.set_editor_property('code','float edge=max(abs(UV.x*2-1),abs(UV.y*2-1));float noise=sin(P.y*.045+sin(P.z*.07)*2)*.08+sin(P.z*.12)*.025;return saturate((.93-edge+noise)*12);');n.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
ins=[]
for name in ['UV','P']:
 i=unreal.CustomInput();i.set_editor_property('input_name',name);ins.append(i)
n.set_editor_property('inputs',ins);ME.connect_material_expressions(uv,'',n,'UV');ME.connect_material_expressions(pos,'',n,'P');ME.connect_material_property(n,'',unreal.MaterialProperty.MP_OPACITY_MASK);ME.recompile_material(m);E.save_loaded_asset(m)
materials={}
for tag,col,rough in [('RoughRock',(.68,.7,.62),.85),('SmoothRock',(.37,.43,.45),.38),('MossyRock',(.24,.39,.12),.96)]:
 name='MI_Patch_'+tag;mi=unreal.load_asset(R+'/'+name) or AT.create_asset(name,R,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew());ME.set_material_instance_parent(mi,m);ME.set_material_instance_vector_parameter_value(mi,'Tint',unreal.LinearColor(*col,1));ME.set_material_instance_scalar_parameter_value(mi,'DryRoughness',rough);E.save_loaded_asset(mi);materials[tag]=mi
for a in A.get_all_level_actors():
 name=a.get_actor_label()
 if name.startswith('Climbing_Fracture_'):A.destroy_actor(a);continue
 if name.startswith(('GripPatch_Visual_','Climbing_Skin_')):
  for tag,mi in materials.items():
   if a.actor_has_tag(tag):a.static_mesh_component.set_material(0,mi)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('CLIFF_SURFACES_BLENDED')
