"""Project-specific font authoring and read-only PIE inventory inspection over MCP."""
import json
from pathlib import Path
import unreal
import toolset_registry

@unreal.uclass()
class AltaiUITools(unreal.ToolsetDefinition):
    """Altai UI font import and inspection of inventory state. Editor only."""

    @toolset_registry.tool_call
    @staticmethod
    def import_ui_font() -> str:
        """Import the project's bundled, licensed heading font as a Font asset."""
        source=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))/'SourceArt/UI/AltaiHeading.ttf'
        if not source.is_file():raise ValueError('Bundled font source not found')
        task=unreal.AssetImportTask()
        task.filename=str(source);task.destination_path='/Game/Altai/UI/Fonts';task.destination_name='F_Heading'
        task.automated=True;task.save=True;task.replace_existing=False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        if not unreal.AltaiEditorLibrary.create_heading_font():raise ValueError("Could not create composite heading font")
        return json.dumps({"face":list(task.imported_object_paths),"font":"/Game/Altai/UI/Fonts/F_Display.F_Display"})

    @toolset_registry.tool_call
    @staticmethod
    def inspect_inventory() -> str:
        """Read the current PIE inventory, cell order, assignments and active popup without mutation."""
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not world:return json.dumps({'running':False})
        screens=unreal.WidgetBlueprintLibrary.get_all_widgets_of_class(world,unreal.AltaiInventoryScreen,False)
        if not screens:return json.dumps({'running':True,'inventory':False})
        screen=screens[0]
        grid=screen.get_editor_property('item_grid')
        result={'screen':screen.get_path_name(),'cells':[], 'slots':{}}
        for cell in grid.get_all_children():
            if not isinstance(cell,unreal.AltaiItemCell):continue
            result['cells'].append({'ref':cell.get_path_name(),'name':str(cell.get_editor_property('item_name').get_text())})
        for prop in ['head_slot','body_slot','hand_slot','quick_slot1','quick_slot2']:
            cell=screen.get_editor_property(prop)
            result['slots'][prop]={'ref':cell.get_path_name(),'name':str(cell.get_editor_property('equipped_name').get_text())}
        return json.dumps(result,ensure_ascii=False)

    @toolset_registry.tool_call
    @staticmethod
    def seed_ui_test_inventory() -> str:
        """Add deterministic drag/sort test items ONLY to a dedicated active 'UI TEST' PIE profile."""
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not world:raise ValueError('PIE is not running')
        sessions=[s for s in unreal.ObjectIterator(unreal.AltaiExpedition) if s.get_path_name().startswith('/Engine/Transient') and str(s.get_active_profile_name()).startswith('UI TEST')]
        if len(sessions)!=1:raise ValueError('A dedicated active UI TEST profile is required')
        s=sessions[0]
        if any(str(i.id)=='UITestFlaskB' for i in s.get_items()):raise ValueError('Fixture already seeded')
        for n in range(25):
            item=unreal.AltaiItem();item.id='UITestFlaskB' if n==0 else 'UITest'+str(n)
            item.name='Запасная фляга' if n==0 else 'Предмет '+str(n)
            item.description='Временный предмет для проверки интерфейса.';item.quantity=n+2
            item.category=unreal.AltaiItemCategory.TOOL if n==0 else unreal.AltaiItemCategory.MATERIAL
            item.quick_access=n==0
            item.set_editor_property('icon',unreal.load_asset('/Game/Altai/UI/Icons/T_Flask' if n==0 else '/Game/Altai/UI/Icons/T_Stone'))
            if not s.add_item(item):raise ValueError('Could not add fixture item')
        return json.dumps({'count':len(s.get_items())})

    @toolset_registry.tool_call
    @staticmethod
    def read_ui_test_loadout() -> str:
        """Read items and assignments of the active dedicated UI TEST PIE profile."""
        sessions=[s for s in unreal.ObjectIterator(unreal.AltaiExpedition) if s.get_path_name().startswith('/Engine/Transient') and str(s.get_active_profile_name()).startswith('UI TEST')]
        if len(sessions)!=1:raise ValueError('A dedicated active UI TEST profile is required')
        s=sessions[0]
        return json.dumps({'items':[{'id':str(i.id),'quantity':i.quantity} for i in s.get_items()], 'loadout':[{'slot':str(e.slot),'id':str(e.item_id)} for e in s.get_loadout()]})
