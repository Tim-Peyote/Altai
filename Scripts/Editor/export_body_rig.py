import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0)
ref=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
# Skeleton editing API exposes reference transforms independently of current animation.
lib=unreal.SkeletonModifier();lib.set_skeletal_mesh(p.mesh.skeletal_mesh_asset)
names=lib.get_all_bone_names();data={str(n):{'parent':str(lib.get_parent_name(n)),'local':str(lib.get_bone_transform(n,False))} for n in names}
Path(unreal.Paths.project_saved_dir(),'body_rig.json').write_text(json.dumps(data,indent=2))
