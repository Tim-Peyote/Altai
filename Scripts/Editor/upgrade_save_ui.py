"""Run once in Unreal Python after building the editor. Widgets remain editable assets."""
import unreal
unreal.AltaiEditorLibrary.upgrade_save_ui()
path = '/Game/Altai/Framework/BP_AltaiPlayerController'
bp = unreal.EditorAssetLibrary.load_asset(path)
cdo = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(path))
for prop, asset in {
    'load_game_class': 'WBP_LoadGame',
    'new_game_class': 'WBP_NewGame',
    'confirm_exit_class': 'WBP_ConfirmExit',
    'hud_class': 'WBP_GameHUD',
}.items():
    cdo.set_editor_property(prop, unreal.EditorAssetLibrary.load_blueprint_class('/Game/Altai/UI/' + asset))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
print('Profile menu assets upgraded')

# Explicit contrast survives saved Slate styles and remains editable in Designer.
bp = unreal.EditorAssetLibrary.load_asset('/Game/Altai/UI/WBP_NewGame')
field = next(x for x in unreal.ObjectIterator(unreal.EditableTextBox)
             if x.get_path_name().startswith(bp.get_path_name() + ':') and x.get_name() == 'ProfileNameInput')
style = field.get_editor_property('widget_style')
style.foreground_color = unreal.SlateColor(specified_color=unreal.LinearColor(0.84, 0.82, 0.76, 1))
style.focused_foreground_color = style.foreground_color
style.background_color = unreal.SlateColor(specified_color=unreal.LinearColor(0.065, 0.075, 0.083, 1))
style.padding = unreal.Margin(12, 10, 12, 10)
field.set_editor_property('widget_style', style)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
