import unreal,json
from pathlib import Path
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
r=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.AltaiWeatherRig)[0]
out={}
for name in ['rain_fx','snow_fx','fog','clouds','atmosphere','sky','sun']:
 c=r.get_editor_property(name);out[name]={'position':str(c.get_world_location()),'active':c.is_active(),'visible':c.is_visible()}
 if name in ['rain_fx','snow_fx']:out[name]['asset']=str(c.get_asset());out[name]['rate']=c.get_variable_float('User.SpawnRate')
 if name=='fog':out[name]['density']=c.get_editor_property('fog_density')
 if name=='clouds':out[name]['material']=str(c.get_editor_property('material'))
Path(unreal.Paths.project_saved_dir(),'lab_runtime_diagnostic.json').write_text(json.dumps(out,indent=2))
