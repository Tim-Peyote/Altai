import unreal,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_landform import height
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);E=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary;actors={a.get_actor_label():a for a in A.get_all_level_actors()};R='/Game/Altai/Environment/Materials';wood=unreal.load_asset(R+'/M_FieldWood')
def box(name,x,y,bottom,top,width,length):
 a=actors.get(name) or A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,(bottom+top)/2));a.set_actor_label(name);a.set_folder_path('10_LoadCourse' if name.startswith('Balance') else '08_Camp/Stations');a.set_actor_location(unreal.Vector(x,y,(bottom+top)/2),False,True);a.set_actor_scale3d(unreal.Vector(width/100,length/100,(top-bottom)/100));a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));a.static_mesh_component.set_material(0,wood);a.static_mesh_component.set_collision_profile_name('BlockAll')
base=height(5400,-4100)
for side in [-1,1]:
 for i in range(4):
  y=-4100+side*(330+i*60);box('Balance_Step_'+str(side)+'_'+str(i),5400,y,height(5400,y)-5,base+70-i*20,120,60)
for y in [-3870,-4330]:box('Balance_Beam_Support_'+str(y),5400,y,height(5400,y)-5,base+73,90,24)
for name in ['Carry_Stick_08kg_Stand','Carry_Crate_18kg_Stand','Carry_Crate_40kg_Stand']:
 a=actors[name];pos=a.get_actor_location();bottom=pos.z-a.get_actor_bounds(False)[1].z
 for i,(dx,dy) in enumerate([(-35,-40),(35,-40),(-35,40),(35,40)]):
  x=pos.x+dx;y=pos.y+dy;box(name+'_Leg'+str(i),x,y,height(x,y)-3,bottom,10,10)
# World-projected normal maps must use a matching world basis on every mesh/patch.
for name in ['M_LabCliff','M_LabCliffPatch']:
 m=unreal.load_asset(R+'/'+name);m.set_editor_property('tangent_space_normal',False)
 tex=next(n for n in (e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer()==m) if isinstance(n,unreal.MaterialExpressionTextureSample) and n.get_editor_property('sampler_type')==unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
 geom=ME.create_material_expression(m,unreal.MaterialExpressionVertexNormalWS);node=ME.create_material_expression(m,unreal.MaterialExpressionCustom);node.set_editor_property('code','float3 t=abs(G.y)<.9?float3(0,1,0):float3(1,0,0);float3 b=normalize(cross(G,t));t=normalize(cross(b,G));return normalize(t*N.x+b*N.y+G*N.z);');node.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
 ins=[]
 for key in ['G','N']:
  v=unreal.CustomInput();v.set_editor_property('input_name',key);ins.append(v)
 node.set_editor_property('inputs',ins);ME.connect_material_expressions(tex,'RGB',node,'N');ME.connect_material_expressions(geom,'',node,'G');ME.connect_material_property(node,'',unreal.MaterialProperty.MP_NORMAL);ME.recompile_material(m);E.save_loaded_asset(m)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('COURSE_AND_NORMALS_FIXED')
