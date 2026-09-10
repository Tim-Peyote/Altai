import unreal,json
from pathlib import Path
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
rows=[]
for x in [1800,2300,2700,3100,3280]:
 hit=unreal.SystemLibrary.line_trace_single(w,unreal.Vector(x,1700,50),unreal.Vector(x,1700,-600),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
 rows.append({'x':x,'hit':str(hit.to_tuple())})
Path(unreal.Paths.project_saved_dir(),'pond_floor.json').write_text(json.dumps(rows,indent=2))
