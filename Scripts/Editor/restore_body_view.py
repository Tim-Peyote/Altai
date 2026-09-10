import unreal,builtins
from altai_lab_tools import context
if hasattr(builtins,'_altai_body_view_handle'):
 unreal.unregister_slate_post_tick_callback(builtins._altai_body_view_handle);del builtins._altai_body_view_handle
restore_body_w,restore_body_pc=context();unreal.GameplayStatics.set_global_time_dilation(restore_body_w,1);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
