"""Continue a paused live recovery to the next inspection phase (PIE only)."""
import unreal,builtins,time,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context()
old=getattr(builtins,'_altai_body_view_handle',None)
if old is not None:unreal.unregister_slate_post_tick_callback(old)
request=Path(unreal.Paths.project_saved_dir())/'body_view_request.json'
options=json.loads(request.read_text()) if request.exists() else {}
if request.exists():request.unlink()
target=options.get('progress',.75);started=time.monotonic();paused=False
unreal.GameplayStatics.set_global_time_dilation(w,.4)
def tick(dt):
 global paused,started
 if not paused and (pc.body_dynamics.recovery_progress>=target or pc.body_dynamics.state==unreal.AltaiBodyState.BALANCED):
  paused=True;started=time.monotonic();unreal.GameplayStatics.set_global_time_dilation(w,.001)
 if time.monotonic()-started>45:
  unreal.unregister_slate_post_tick_callback(handle);unreal.GameplayStatics.set_global_time_dilation(w,1);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
handle=unreal.register_slate_post_tick_callback(tick)
builtins._altai_body_view_handle=handle
