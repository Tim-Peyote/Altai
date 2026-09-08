"""One-time editor authoring. Creates missing assets; preserves existing compositions.
Run with Unreal Editor -ExecutePythonScript=<absolute path to this file>.
"""
import unreal
from pathlib import Path

ROOT = '/Game/Altai'
assets = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def blueprint(name, folder, parent):
    path = f'{ROOT}/{folder}/{name}'
    existing = unreal.load_asset(path)
    if existing:
        return existing
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', parent)
    return assets.create_asset(name, f'{ROOT}/{folder}', unreal.Blueprint, factory)

def cls(bp):
    return unreal.load_class(None, bp.get_path_name() + '_C')

def defaults(bp):
    return unreal.get_default_object(cls(bp))

def save(bp):
    unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)

unreal.AltaiEditorLibrary.create_ui_assets()
unreal.AltaiEditorLibrary.upgrade_save_ui()

# Provisional supplies are editable data, separate from inventory logic.
if not unreal.EditorAssetLibrary.does_asset_exist(ROOT+'/Items/DA_StartingInventory'):
    factory=unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class',unreal.AltaiInventoryPreset)
    preset=assets.create_asset('DA_StartingInventory',ROOT+'/Items',unreal.AltaiInventoryPreset,factory)
    entries=[]
    for identity,name,description,quantity in [('Stone','Камень','Обычный гладкий камень. Материал.',3),('Fiber','Волокно','Сухое растительное волокно. Материал.',2),('Flask','Пустая фляга','Небольшая ёмкость. Пока пуста.',1)]:
        item=unreal.AltaiItem()
        for key,value in {'id':identity,'name':name,'description':description,'quantity':quantity}.items(): item.set_editor_property(key,value)
        entries.append(item)
    preset.set_editor_property('items',entries)
    save(preset)



character = blueprint('BP_AltaiCharacter', 'Player', unreal.AltaiCharacter)
c = defaults(character)
c.mesh.set_skeletal_mesh_asset(unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'))
c.mesh.set_relative_location(unreal.Vector(0, 0, -96), False, False)
c.mesh.set_relative_rotation(unreal.Rotator(pitch=0, yaw=-90, roll=0), False, False)
c.mesh.set_anim_instance_class(unreal.load_class(None, '/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C'))
c.set_editor_property('mapping_context', unreal.load_asset('/Game/Input/IMC_Default'))
c.set_editor_property('move_action', unreal.load_asset('/Game/Input/Actions/IA_Move'))
c.set_editor_property('look_action', unreal.load_asset('/Game/Input/Actions/IA_MouseLook'))
c.set_editor_property('jump_action', unreal.load_asset('/Game/Input/Actions/IA_Jump'))
save(character)

controller = blueprint('BP_AltaiPlayerController', 'Framework', unreal.AltaiPlayerController)
c = defaults(controller)
for prop, name in [('main_menu_class', 'MainMenu'), ('pause_class', 'Pause'), ('inventory_class', 'FieldInventory' if unreal.EditorAssetLibrary.does_asset_exist(ROOT+'/UI/WBP_FieldInventory') else 'Inventory'), ('settings_class', 'Settings'), ('load_game_class', 'LoadGame'), ('new_game_class', 'NewGame'), ('confirm_exit_class', 'ConfirmExit'), ('hud_class', 'GameHUD')]:
    c.set_editor_property(prop, unreal.load_class(None, f'{ROOT}/UI/WBP_{name}.WBP_{name}_C'))
save(controller)

menu_mode = blueprint('BP_MenuMode', 'Framework', unreal.AltaiMenuMode)
world_mode = blueprint('BP_WorldMode', 'Framework', unreal.AltaiWorldMode)
for bp in (menu_mode, world_mode):
    c = defaults(bp)
    c.set_editor_property('player_controller_class', cls(controller))
    c.set_editor_property('default_pawn_class', cls(character) if bp == world_mode else None)
    save(bp)

cube = unreal.load_asset('/Engine/BasicShapes/Cube')
material = unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial')

def spawn(actor_class, label, location=(0,0,0), rotation=(0,0,0)):
    a = actors.spawn_actor_from_class(actor_class, unreal.Vector(*location), unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    a.set_actor_label(label)
    return a

def box(label, location, scale):
    a = spawn(unreal.StaticMeshActor, label, location)
    a.static_mesh_component.set_static_mesh(cube)
    a.static_mesh_component.set_material(0, material)
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_folder_path('Blockout')
    return a

for name, mode in [('L_MainMenu', menu_mode), ('L_World', world_mode)]:
    path = f'{ROOT}/Maps/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        continue
    if not levels.new_level(path):
        raise RuntimeError('Cannot create ' + path)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', cls(mode))
    world.get_world_settings().set_editor_property('kill_z', -2000)
    sun = spawn(unreal.DirectionalLight, 'Sun', (0,0,800), (-35,-30,0))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('intensity', 75000.0)
    sun.light_component.set_editor_property('atmosphere_sun_light', True)
    sun.set_folder_path('Sky')
    sky = spawn(unreal.SkyAtmosphere, 'SkyAtmosphere')
    sky.set_folder_path('Sky')
    skylight = spawn(unreal.SkyLight, 'SkyLight', (0,0,300))
    skylight.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    skylight.light_component.set_editor_property('real_time_capture', True)
    skylight.set_folder_path('Sky')
    fog = spawn(unreal.ExponentialHeightFog, 'HorizonFog')
    fog.set_folder_path('Sky')
    pp = spawn(unreal.PostProcessVolume, 'Exposure')
    pp.set_editor_property('unbound', True)
    settings = pp.get_editor_property('settings')
    settings.set_editor_property('override_auto_exposure_method', True)
    settings.set_editor_property('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL)
    settings.set_editor_property('override_auto_exposure_bias', True)
    settings.set_editor_property('auto_exposure_bias', 0.0)
    for key,value in {'auto_exposure_apply_physical_camera_exposure':True,'camera_iso':100.0,'camera_shutter_speed':125.0,'depth_of_field_fstop':8.0}.items():
        settings.set_editor_property(key,value)
        settings.set_editor_property('override_'+key,True)
    pp.set_editor_property('settings', settings)
    pp.set_folder_path('Sky')
    if name == 'L_World':
        box('Ground', (0,0,-50), (100,100,1))
        for label, loc, scale in [('North', (0,4950,80),(100,1,2.6)), ('South',(0,-4950,80),(100,1,2.6)), ('East',(4950,0,80),(1,100,2.6)), ('West',(-4950,0,80),(1,100,2.6))]:
            box('Boundary_'+label,loc,scale)
        spawn(unreal.PlayerStart, 'PlayerStart', (0,0,110))
    else:
        spawn(unreal.CameraActor, 'MenuCamera', (0,0,200))
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-800,-800,600), unreal.Rotator(pitch=-25,yaw=45,roll=0))
    levels.save_current_level()

levels.load_level(f'{ROOT}/Maps/L_MainMenu')
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
Path(unreal.Paths.project_saved_dir(), 'foundation_created.txt').write_text('Created UI, player, framework and maps.\n')
unreal.log('ALTAI_FOUNDATION_CREATED')
