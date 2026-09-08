import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);g=unreal.GameUserSettings.get_game_user_settings()
keys=['view_distance','shadow','global_illumination','reflection','anti_aliasing','texture','visual_effect','post_processing','foliage','shading'];r={'graphics':{k:getattr(g,'get_'+k+'_quality')() for k in keys}}
r['graphics'].update(resolution=g.get_resolution_scale_information_ex()[1],fps=g.get_frame_rate_limit(),vsync=g.is_v_sync_enabled())
r.update(panel_open=pc.developer_panel is not None,cursor=pc.get_editor_property('show_mouse_cursor'),ignore_move=pc.is_move_input_ignored(),ignore_look=pc.is_look_input_ignored(),wall_input=pc.wall_climbing.input_from_player,paused=unreal.GameplayStatics.is_game_paused(w),weather=pc.weather.preset_index,hour=pc.weather.hour,automatic_weather=pc.weather.automatic_weather,cycle_time=pc.weather.cycle_time,lightning=pc.weather.lightning_enabled,mass=p.character_movement.mass,monitor=pc.show_diagnostics,first_person=p.first_person)
Path(unreal.Paths.project_saved_dir(),'developer_panel_state.json').write_text(json.dumps(r,indent=2));unreal.log('DEVELOPER_PANEL_STATE '+json.dumps(r))
