"""Author the editable inventory widgets and studio. Run after building AltaiEditor."""
import unreal
from pathlib import Path
assets=unreal.AssetToolsHelpers.get_asset_tools()
lib=unreal.EditorAssetLibrary
folder='/Game/Altai/UI/Preview'
lib.make_directory(folder)
def save(asset):lib.save_loaded_asset(asset,False)
rt=lib.load_asset(folder+'/RT_InventoryPreview') if lib.does_asset_exist(folder+'/RT_InventoryPreview') else assets.create_asset('RT_InventoryPreview',folder,unreal.TextureRenderTarget2D,unreal.TextureRenderTargetFactoryNew())
rt.set_editor_property('size_x',640);rt.set_editor_property('size_y',896)
rt.set_editor_property('render_target_format',unreal.TextureRenderTargetFormat.RTF_RGBA8)
rt.set_editor_property('clear_color',unreal.LinearColor(0,0,0,1));save(rt)
material=lib.load_asset(folder+'/M_InventoryPreview') if lib.does_asset_exist(folder+'/M_InventoryPreview') else None
if not material:
 material=assets.create_asset('M_InventoryPreview',folder,unreal.Material,unreal.MaterialFactoryNew())
 material.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI)
 texture=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionTextureSampleParameter2D,-300,0)
 texture.set_editor_property('texture',rt);texture.set_editor_property('parameter_name','Portrait')
 texture.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
 unreal.MaterialEditingLibrary.connect_material_property(texture,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 unreal.MaterialEditingLibrary.recompile_material(material);save(material)
rigpath=folder+'/BP_InventoryPreviewRig'
if not lib.does_asset_exist(rigpath):
 factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.AltaiInventoryPreview)
 rig=assets.create_asset('BP_InventoryPreviewRig',folder,unreal.Blueprint,factory)
else:rig=lib.load_asset(rigpath)
cls=lib.load_blueprint_class(rigpath);cdo=unreal.get_default_object(cls)
capture=cdo.get_editor_property('capture');capture.set_editor_property('texture_target',rt)
capture.set_editor_property('always_persist_rendering_state',True)
capture.set_editor_property('show_flag_settings',[
 unreal.EngineShowFlagsSetting(show_flag_name='SkyLighting',enabled=False),
 unreal.EngineShowFlagsSetting(show_flag_name='EyeAdaptation',enabled=True)])
pp=capture.get_editor_property('post_process_settings')
pp.override_auto_exposure_bias=True;pp.auto_exposure_bias=2
pp.override_bloom_intensity=True;pp.bloom_intensity=0
pp.override_auto_exposure_apply_physical_camera_exposure=True;pp.auto_exposure_apply_physical_camera_exposure=False
capture.set_editor_property('post_process_settings',pp);save(rig)
unreal.AltaiEditorLibrary.create_inventory_assets()
unreal.AltaiEditorLibrary.polish_inventory_assets()
controller=lib.load_asset('/Game/Altai/Framework/BP_AltaiPlayerController')
c=unreal.get_default_object(lib.load_blueprint_class('/Game/Altai/Framework/BP_AltaiPlayerController'))
c.set_editor_property('inventory_class',lib.load_blueprint_class('/Game/Altai/UI/WBP_FieldInventory'))
unreal.BlueprintEditorLibrary.compile_blueprint(controller);save(controller)
# Keep owned quantities intact: only the editable definitions receive new metadata/icons.
source=Path(unreal.Paths.project_dir())/'SourceArt/Inventory'
for name in ['Stone','Fiber','Flask']:
 path='/Game/Altai/UI/Icons/T_'+name
 if not lib.does_asset_exist(path):
  task=unreal.AssetImportTask();task.filename=str(source/('T_'+name+'.png'));task.destination_path='/Game/Altai/UI/Icons';task.automated=True;task.save=True
  assets.import_asset_tasks([task])
 texture=lib.load_asset(path)
 texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
 texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);save(texture)
preset=lib.load_asset('/Game/Altai/Items/DA_StartingInventory');items=list(preset.get_editor_property('items'))
for item in items:
 name=str(item.id)
 if name in ['Stone','Fiber','Flask']:
  item.set_editor_property('icon',lib.load_asset('/Game/Altai/UI/Icons/T_'+name))
 if name=='Flask':item.category=unreal.AltaiItemCategory.TOOL;item.quick_access=True
preset.set_editor_property('items',items);save(preset)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
map_path='/Game/Altai/Maps/L_Preview_Inventory'
if not lib.does_asset_exist(map_path):
 if not levels.new_level(map_path):raise RuntimeError('Cannot create inventory studio map')
 actor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(cls,unreal.Vector(),unreal.Rotator())
 actor.set_actor_label('Inventory Preview Studio');actor.refresh_preview()
 unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(330,0,100),unreal.Rotator(pitch=0,yaw=180,roll=0))
 levels.save_current_level()
levels.load_level('/Game/Altai/Maps/L_MainMenu')
print('Inventory UI, definitions, icons and separate preview map saved.')
