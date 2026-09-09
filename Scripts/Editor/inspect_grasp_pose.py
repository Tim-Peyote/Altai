"""Read-only pose/contact diagnostics for a currently held PIE object."""
import unreal,json
from pathlib import Path
from altai_lab_tools import context
inspect_w,inspect_pc=context();inspect_p=unreal.GameplayStatics.get_player_pawn(inspect_w,0);inspect_h=inspect_pc.hands
inspect_r={'held':inspect_h.held.get_owner().get_actor_label() if inspect_h.held else None,'wrist_angle_error':inspect_h.wrist_tracking_error,'forearm_roll':inspect_h.forearm_roll,'angles':list(inspect_h.grip_angles.to_tuple()),'bones':{}}
for inspect_side in ['r','l']:
 for inspect_b in ['upperarm','lowerarm','hand','thumb_01','thumb_02','thumb_03','index_01','index_02','index_03','middle_03','ring_03','pinky_03']:
  inspect_name=inspect_b+'_'+inspect_side;inspect_pos=inspect_p.mesh.get_socket_location(inspect_name);inspect_row={'position':list(inspect_pos.to_tuple())}
  if inspect_h.held:
   inspect_row['object_local']=list(unreal.MathLibrary.inverse_transform_location(inspect_h.held.get_owner().get_actor_transform(),inspect_pos).to_tuple());inspect_row['collision_distance']=str(inspect_h.held.get_closest_point_on_collision(inspect_pos))
  inspect_r['bones'][inspect_name]=inspect_row
Path(unreal.Paths.project_saved_dir(),'grasp_pose_inspection.json').write_text(json.dumps(inspect_r,indent=2))
