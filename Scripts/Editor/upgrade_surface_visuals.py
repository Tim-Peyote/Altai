import unreal
EA=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();R='/Game/Altai/Environment/Materials'
for name,tint in [('M_MudStepParticle',(.20,.115,.055)),('M_SnowStepParticle',(.7,.8,.84))]:
 m=unreal.load_asset(R+'/'+name)
 if not m:
  m=AT.create_asset(name,R,unreal.Material,unreal.MaterialFactoryNew())
  c=ME.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);c.constant=unreal.LinearColor(*tint,1);ME.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
  r=ME.create_material_expression(m,unreal.MaterialExpressionConstant);r.r=.85;ME.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
  ME.recompile_material(m);EA.save_loaded_asset(m)
 for e in unreal.ObjectIterator(unreal.MaterialExpressionConstant3Vector):
  if e.get_outer()==m:e.set_editor_property('constant',unreal.LinearColor(*tint,1))
 ME.recompile_material(m);EA.save_loaded_asset(m)
# Daylight exposure previously made the unlit rings almost invisible.
m=unreal.load_asset(R+'/M_WaterRipple')
ME.delete_all_material_expressions(m)
def node(cls):return ME.create_material_expression(m,cls)
uv=node(unreal.MaterialExpressionTextureCoordinate);mask=node(unreal.MaterialExpressionCustom);mask.set_editor_property('code','float r=length(UV-.5)*2;return (1-smoothstep(.012,.035,abs(r-.68)))*.45*saturate((95-Radius)/65);');mask.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1);i=unreal.CustomInput();i.set_editor_property('input_name','UV');j=unreal.CustomInput();j.set_editor_property('input_name','Radius');mask.set_editor_property('inputs',[i,j]);radius=node(unreal.MaterialExpressionObjectRadius);ME.connect_material_expressions(radius,'',mask,'Radius');ME.connect_material_expressions(uv,'',mask,'UV');ME.connect_material_property(mask,'',unreal.MaterialProperty.MP_OPACITY)
c=node(unreal.MaterialExpressionConstant3Vector);c.constant=unreal.LinearColor(.20,.26,.28,1);eye=node(unreal.MaterialExpressionEyeAdaptation);div=node(unreal.MaterialExpressionDivide);ME.connect_material_expressions(c,'',div,'A');ME.connect_material_expressions(eye,'',div,'B');ME.connect_material_property(div,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
ME.recompile_material(m);EA.save_loaded_asset(m)
# Stronger footprint contrast with a rim and clearly separated heel/toe.
for name in ['M_FootprintMud','M_FootprintSnow']:
 m=unreal.load_asset(R+'/'+name)
 for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom):
  if e.get_outer()==m:
   e.set_editor_property('code',e.get_editor_property('code').replace('tread*.8','tread*.95'))
 if name=='M_FootprintSnow':
  for e in unreal.ObjectIterator(unreal.MaterialExpressionConstant3Vector):
   if e.get_outer()==m:e.set_editor_property('constant',unreal.LinearColor(.075,.10,.12,1))
 ME.recompile_material(m);EA.save_loaded_asset(m)
unreal.log('SURFACE_VISUALS_UPGRADED')
