"""Bake authored swimming motion into native Unreal animation assets."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir());tools=unreal.AssetToolsHelpers.get_asset_tools()
mesh=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple');folder='/Game/Altai/Player/Animations'
for label in ['SwimBreaststroke','SwimEasy','SwimTread','SwimDrown']:
 data=json.loads((root/'SourceArt/Motion/CMU125'/(label+'.json')).read_text());name='A_'+label
 asset=unreal.load_asset(folder+'/'+name)
 if not asset:
  factory=unreal.AnimSequenceFactory();factory.target_skeleton=mesh.skeleton;asset=tools.create_asset(name,folder,unreal.AnimSequence,factory)
 controller=asset.controller
 controller.open_bracket('Bake authored swimming',False);controller.remove_all_bone_tracks(False);controller.set_frame_rate(unreal.FrameRate(30,1),False);controller.set_number_of_frames(unreal.FrameNumber(len(data['frames'])-1),False)
 for bone in data['frames'][0]:
  controller.add_bone_curve(bone,False)
  controller.set_bone_track_keys(bone,[unreal.Vector(*f[bone]['p']) for f in data['frames']],[unreal.Quat(*f[bone]['q']) for f in data['frames']],[unreal.Vector(1,1,1)]*len(data['frames']),False)
 controller.close_bracket(False)
 if not unreal.EditorAssetLibrary.save_loaded_asset(asset,False):raise RuntimeError("Could not save "+name)
 unreal.log('ALTAI_MOTION_IMPORTED '+name)
