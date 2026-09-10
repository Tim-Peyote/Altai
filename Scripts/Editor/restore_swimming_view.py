import unreal,builtins
from altai_lab_tools import context
w,pc=context();h=getattr(builtins,'_altai_swim_view_handle',None)
if h is not None:
 unreal.unregister_slate_post_tick_callback(h);del builtins._altai_swim_view_handle
unreal.GameplayStatics.set_global_time_dilation(w,1);pc.swimming.input_from_player=True
