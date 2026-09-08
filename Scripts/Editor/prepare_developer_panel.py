import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0)
p.set_actor_location(unreal.Vector(5130,-5550,247),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=165,roll=0));p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=165,roll=0),True)
unreal.AltaiEditorLibrary.set_physics_test_mode(True)
# Capture current graphics before the UI apply/revert test; do not change the user's profile permanently.
g=unreal.GameUserSettings.get_game_user_settings()
keys=['view_distance','shadow','global_illumination','reflection','anti_aliasing','texture','visual_effect','post_processing','foliage','shading']
data={k:getattr(g,'get_'+k+'_quality')() for k in keys};data['resolution']=g.get_resolution_scale_information_ex()[1];data['fps']=g.get_frame_rate_limit();data['vsync']=g.is_v_sync_enabled();data['screen_percentage']=unreal.SystemLibrary.get_console_variable_float_value('r.ScreenPercentage')
Path(unreal.Paths.project_saved_dir(),'developer_panel_graphics_original.json').write_text(json.dumps(data,indent=2))
unreal.log('DEVELOPER_PANEL_PREPARED')
