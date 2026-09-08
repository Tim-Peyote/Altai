import unreal,json
from pathlib import Path
v=json.loads(Path(unreal.Paths.project_saved_dir(),'developer_panel_graphics_original.json').read_text());g=unreal.GameUserSettings.get_game_user_settings()
for k in ['view_distance','shadow','global_illumination','reflection','anti_aliasing','texture','visual_effect','post_processing','foliage','shading']:getattr(g,'set_'+k+'_quality')(v[k])
g.set_resolution_scale_value_ex(v['resolution']);g.set_frame_rate_limit(v['fps']);g.set_v_sync_enabled(v['vsync']);g.apply_non_resolution_settings();g.save_settings();
if v['resolution']==0:unreal.AltaiEditorLibrary.restore_automatic_resolution(v.get('screen_percentage',100))
unreal.log('DEVELOPER_GRAPHICS_RESTORED')
