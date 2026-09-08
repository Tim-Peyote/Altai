"""Create the character lab once; never rebuild an existing, hand-edited map."""
import unreal
from pathlib import Path

ROOT = '/Game/Altai/Debug'
MAP = ROOT + '/Maps/L_CharacterLab'
assets = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def spawn(kind, name, position, folder, rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_class(kind, unreal.Vector(*position),
        unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    actor.set_actor_label(name)
    actor.set_folder_path(folder)
    return actor


def box(name, position, size, folder, rotation=(0, 0, 0), mass=None):
    actor = spawn(unreal.StaticMeshActor, name, position, folder, rotation)
    mesh = actor.static_mesh_component
    mesh.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    actor.set_actor_scale3d(unreal.Vector(*(v / 100 for v in size)))
    mesh.set_collision_profile_name('PhysicsActor' if mass else 'BlockAll')
    if mass:
        mesh.set_mobility(unreal.ComponentMobility.MOVABLE)
        mesh.set_mass_override_in_kg(unreal.Name('None'), mass, True)
        mesh.set_simulate_physics(True)
    return actor


def sign(name, caption, position, folder):
    actor = spawn(unreal.TextRenderActor, name, position, folder)
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    component.set_text(caption)
    component.set_world_size(24)
    component.set_text_render_color(unreal.Color(220, 190, 125, 255))
    return actor


def exposure():
    actor = spawn(unreal.PostProcessVolume, 'Lab_Exposure', (0, 0, 0), '00_Environment')
    actor.set_editor_property('unbound', True)
    settings = actor.get_editor_property('settings')
    for key, value in {
        'auto_exposure_method': unreal.AutoExposureMethod.AEM_MANUAL,
        'auto_exposure_bias': 0.0,
        'auto_exposure_apply_physical_camera_exposure': True,
        'camera_iso': 100.0,
        'camera_shutter_speed': 125.0,
        'depth_of_field_fstop': 8.0,
    }.items():
        settings.set_editor_property(key, value)
        settings.set_editor_property('override_' + key, True)
    actor.set_editor_property('settings', settings)


def create():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        unreal.log('ALTAI_CHARACTER_LAB_EXISTS: preserved existing map')
        return
    pawn = unreal.load_class(None, '/Game/Altai/Player/BP_AltaiCharacter.BP_AltaiCharacter_C')
    if not pawn:
        raise RuntimeError('Missing existing Altai character')
    mode_path = ROOT + '/Framework/BP_CharacterLabMode'
    mode = unreal.load_asset(mode_path)
    if not mode:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property('parent_class', unreal.GameModeBase)
        mode = assets.create_asset('BP_CharacterLabMode', ROOT + '/Framework', unreal.Blueprint, factory)
        defaults = unreal.get_default_object(unreal.load_class(None, mode_path + '.BP_CharacterLabMode_C'))
        defaults.set_editor_property('default_pawn_class', pawn)
        # Standard controller deliberately avoids expedition UI/profile initialization.
        defaults.set_editor_property('player_controller_class', unreal.PlayerController)
        unreal.EditorAssetLibrary.save_loaded_asset(mode)
    if not levels.new_level(MAP):
        raise RuntimeError('Cannot create lab')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', unreal.load_class(None, mode_path + '.BP_CharacterLabMode_C'))
    world.get_world_settings().set_editor_property('kill_z', -2000)
    box('Floor_40m', (0, 0, -25), (4000, 4000, 50), '00_Environment')
    for name, location, size in [
        ('North', (0, 2000, 100), (4050, 50, 200)),
        ('South', (0, -2000, 100), (4050, 50, 200)),
        ('East', (2000, 0, 100), (50, 4000, 200)),
        ('West', (-2000, 0, 100), (50, 4000, 200))]:
        box('Boundary_' + name, location, size, '00_Environment')
    sun = spawn(unreal.DirectionalLight, 'Sun', (0, 0, 900), '00_Environment', (-40, -35, 0))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('intensity', 75000.0)
    sun.light_component.set_editor_property('atmosphere_sun_light', True)
    spawn(unreal.SkyAtmosphere, 'Sky', (0, 0, 0), '00_Environment')
    sky = spawn(unreal.SkyLight, 'SkyLight', (0, 0, 400), '00_Environment')
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('real_time_capture', True)
    exposure()
    spawn(unreal.PlayerStart, 'Lab_Start', (1600, 0, 110), '01_Start', (0, 180, 0))
    sign('Welcome', 'ALTAI / CHARACTER LAB\nWASD - MOVE / SPACE - JUMP\nESC - STOP PIE / RESTART PLAY TO RESET', (1200, -450, 190), '01_Start')
    # Centimetre markers make scale and movement distances inspectable.
    for i in range(11):
        box('Metre_%02d' % i, (1000 - i * 100, 0, 1), (4, 90, 2), '02_Movement')
    sign('Movement', '01 / MOVEMENT\n10 m STRAIGHT / STEPS 15 cm / SLOPE 20 deg', (300, -1750, 230), '02_Movement')
    for i in range(6):
        height = 15 * (i + 1)
        box('Step_%02d' % i, (-100 - i * 40, -1400, height / 2), (40, 220, height), '02_Movement')
    box('Ramp_20deg', (-650, -1400, 115), (600, 220, 30), '02_Movement', (20, 0, 0))
    sign('Camera', '02 / CAMERA CLEARANCE\n120 cm PASSAGE / 150 cm LOW GATE\nFIRST PERSON + CROUCH: PLANNED', (300, 500, 230), '03_Camera')
    for y in (760, 920):
        box('PassageWall_' + str(y), (-300, y, 150), (600, 40, 300), '03_Camera')
    for y in (1230, 1490):
        box('LowGatePost_' + str(y), (-900, y, 85), (40, 40, 170), '03_Camera')
    box('LowGateTop_Clearance150', (-900, 1360, 170), (80, 300, 40), '03_Camera')
    sign('Traversal', '03 / HAND CONTACT + TRAVERSAL\nLEDGES 60 / 100 / 140 cm\nGRAB + MANTLE: PLANNED', (-1100, -1700, 250), '04_Traversal')
    for i, height in enumerate((60, 100, 140)):
        box('Ledge_%dcm' % height, (-1550, -1350 + i * 350, height / 2), (350, 250, height), '04_Traversal')
    sign('Physics', '04 / PHYSICS + HANDS\nLOOSE BLOCKS: 1 / 5 / 20 kg\nHAND GRAB: PLANNED', (-1100, 250, 260), '05_Physics')
    box('Worktop_90cm', (-1500, 900, 85), (250, 600, 10), '05_Physics')
    for x in (-1590, -1410):
        for y in (670, 1130):
            box('TableLeg_%s_%s' % (x, y), (x, y, 40), (15, 15, 80), '05_Physics')
    for i, mass in enumerate((1, 5, 20)):
        box('PhysicsBlock_%dkg' % mass, (-1500, 700 + i * 180, 115), (30, 30, 30), '05_Physics', mass=mass)
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(
        unreal.Vector(3100, -3400, 3000), unreal.Rotator(pitch=-35, yaw=135, roll=0))
    if not levels.save_current_level():
        raise RuntimeError('Lab save failed')
    Path(unreal.Paths.project_saved_dir(), 'character_lab_created.txt').write_text(MAP + '\n')
    unreal.log('ALTAI_CHARACTER_LAB_CREATED')


create()
