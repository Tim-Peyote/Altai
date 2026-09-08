import unreal,json
from pathlib import Path
E=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary
D=json.loads(Path(unreal.Paths.project_saved_dir(),'environment_assets.json').read_text())
R='/Game/Altai/Environment/Materials';name='M_GripStone'
m=unreal.load_asset(R+'/'+name) or unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,R,unreal.Material,unreal.MaterialFactoryNew())
ME.delete_all_material_expressions(m)
def node(cls,**props):
 n=ME.create_material_expression(m,getattr(unreal,cls))
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def link(a,b,p,out=''):ME.connect_material_expressions(a,out,b,p)
def prop(a,p,out=''):ME.connect_material_property(a,out,getattr(unreal.MaterialProperty,p))
uv=node('MaterialExpressionTextureCoordinate',u_tiling=2.,v_tiling=2.)
tex=node('MaterialExpressionTextureSample',texture=unreal.load_asset(D['textures']['rock_face_03']['Diffuse']))
link(uv,tex,'UVs')
tint=node('MaterialExpressionVectorParameter',parameter_name='Tint',default_value=unreal.LinearColor(1,1,1,1))
mul=node('MaterialExpressionMultiply');link(tex,mul,'A','RGB');link(tint,mul,'B')
wet=node('MaterialExpressionScalarParameter',parameter_name='Wetness',default_value=0.)
fade=node('MaterialExpressionLinearInterpolate',const_a=1.,const_b=.45);link(wet,fade,'Alpha')
col=node('MaterialExpressionMultiply');link(mul,col,'A');link(fade,col,'B');prop(col,'MP_BASE_COLOR')
dry=node('MaterialExpressionScalarParameter',parameter_name='DryRoughness',default_value=.85)
rough=node('MaterialExpressionLinearInterpolate',const_b=.2);link(dry,rough,'A');link(wet,rough,'Alpha');prop(rough,'MP_ROUGHNESS')
n=node('MaterialExpressionTextureSample',texture=unreal.load_asset(D['textures']['rock_face_03']['nor_dx']),sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
link(uv,n,'UVs');prop(n,'MP_NORMAL','RGB')
ME.recompile_material(m);E.save_loaded_asset(m)
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in A.get_all_level_actors():
 if isinstance(a,unreal.StaticMeshActor) and (a.get_actor_label().startswith('Loose_Stone_') or a.actor_has_tag(unreal.Name('AltaiClimbable'))):
  a.static_mesh_component.set_material(0,m)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('CONTACT_MATERIALS_FIXED')
