"""Run with Blender --background --python. Keep Poly Haven UVs; split at authored joints.
OBJ importer converts handedness by reversing Y, so file Y is opposite UE Y.
"""
import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]/'SourceArt/Environment/InteractionAssets';OUT=ROOT/'Prepared';OUT.mkdir(exist_ok=True)
manifest=[]
def export(name,objects,scale,pivot=(0,0,0),**meta):
 verts=[];uvs=[];faces=[]
 for o in objects:
  m=o.data;m.calc_loop_triangles();base=len(verts)
  for v in m.vertices:
   p=o.matrix_world@v.co;p=Vector((-p.y*100*scale,p.x*100*scale,p.z*100*scale))-Vector(pivot)
   verts.append(tuple(p))
  for tri in m.loop_triangles:
   f=[]
   for vi,li in zip(tri.vertices,tri.loops):
    uv=m.uv_layers.active.data[li].uv if m.uv_layers.active else (0,0);uvs.append(tuple(uv));f.append((base+vi+1,len(uvs)))
   faces.append(f[::-1])
 with (OUT/(name+'.obj')).open('w') as f:
  f.write('o '+name+'\n')
  for x,y,z in verts:f.write(f'v {x:.6f} {-y:.6f} {z:.6f}\n')
  for u,v in uvs:f.write(f'vt {u:.7f} {v:.7f}\n')
  for face in faces:f.write('f '+' '.join(f'{v}/{uv}' for v,uv in face)+'\n')
 bounds=[[min(v[i] for v in verts),max(v[i] for v in verts)] for i in range(3)]
 manifest.append(dict(name=name,source=ident,pivot=pivot,bounds=bounds,**meta))
for ident in ['treasure_chest','vintage_wooden_drawer_01','large_castle_door']:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(ROOT/ident/(ident+'.gltf')))
 objects=[o for o in bpy.data.objects if o.type=='MESH']
 if ident=='treasure_chest':
  export('SM_PH_ChestBase',[o for o in objects if 'lid' not in o.name and 'lock' not in o.name],1)
  pivot=(-25,0,42)
  export('SM_PH_ChestLid',[o for o in objects if 'lid' in o.name or 'lock' in o.name],1,pivot,axis=[0,-1,0],travel=100,mass=9,grip=[52,0,-7])
 elif ident=='vintage_wooden_drawer_01':
  export('SM_PH_Cabinet',[o for o in objects if 'body' in o.name],1.5)
  for i in range(1,7):
   o=next(o for o in objects if f'drawer0{i}' in o.name)
   vs=[o.matrix_world@v.co for v in o.data.vertices]
   y=(min(v.x for v in vs)+max(v.x for v in vs))*75;z=(min(v.z for v in vs)+max(v.z for v in vs))*75
   export(f'SM_PH_Drawer{i}',[o],1.5,(0,y,z),axis=[1,0,0],travel=36,mass=3+i*.6,grip=[36,0,0],sliding=True)
 else:
  export('SM_PH_DoorFrame',[o for o in objects if 'frame' in o.name],.8)
  export('SM_PH_DoorLeft',[o for o in objects if 'left' in o.name],.8,(0,-80,3),axis=[0,0,-1],travel=105,mass=24,grip=[9,69,100])
  export('SM_PH_DoorRight',[o for o in objects if 'right' in o.name],.8,(0,73,3),axis=[0,0,1],travel=105,mass=30,grip=[19,-69,100])
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('FURNITURE_EXPORT_DONE',len(manifest))
