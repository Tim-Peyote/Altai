"""Solid collision for the raised seams and tapering sides, outside the three climb lanes."""
import unreal,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_test_geometry import Mesh
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);E=unreal.EditorAssetLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();actors={a.get_actor_label():a for a in A.get_all_level_actors()}
folder=Path(unreal.Paths.project_dir(),'SourceArt/Environment/FieldTests');R='/Game/Altai/Environment/Geometry'
shapes={'SM_CliffGapCollision':[(0,-150),(180,0),(0,150),(-600,150),(-600,-150)],'SM_CliffEdgeCollision':[(0,0),(-160,100),(-160,250),(-600,250),(-600,0)]}
for name,poly in shapes.items():
 path=R+'/'+name
 if not E.does_asset_exist(path):
  m=Mesh();lo=[(x,y,-150) for x,y in poly];hi=[(x,y,1850) for x,y in poly]
  m.face(list(reversed(lo)));m.face(hi)
  for i in range(len(poly)):j=(i+1)%len(poly);m.face([lo[i],lo[j],hi[j],hi[i]])
  m.write(folder/(name+'.obj'));t=unreal.AssetImportTask();t.filename=str(folder/(name+'.obj'));t.destination_path=R;t.destination_name=name;t.automated=True;t.save=True;AT.import_asset_tasks([t])
 mesh=unreal.load_asset(path);mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE);E.save_loaded_asset(mesh)
for name,geo,y,flip in [('Cliff_Solid_Gap0','SM_CliffGapCollision',-3700,1),('Cliff_Solid_Gap1','SM_CliffGapCollision',-4900,1),('Cliff_Solid_Edge0','SM_CliffEdgeCollision',-2650,-1),('Cliff_Solid_Edge1','SM_CliffEdgeCollision',-5950,1)]:
 a=actors.get(name) or A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(3560,y,35));a.set_actor_label(name);a.set_folder_path('03_ClimbingRock/Collision');a.static_mesh_component.set_static_mesh(unreal.load_asset(R+'/'+geo));a.static_mesh_component.set_collision_profile_name('BlockAll');a.set_actor_scale3d(unreal.Vector(1,flip,1));a.set_actor_hidden_in_game(True)
actors['Climbing_BackCollision'].set_actor_scale3d(unreal.Vector(6.4,33,18))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for y in [-3700,-4900,-2500,-6100]:
 hit=unreal.SystemLibrary.line_trace_single(w,unreal.Vector(4000,y,500),unreal.Vector(3200,y,500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
 assert hit is not None, 'Missing cliff collision at '+str(y)
 unreal.log('CLIFF_COLLISION_TRACE '+str(y)+' '+str(hit))
unreal.log('CLIFF_SIDE_COLLISION_FITTED')
