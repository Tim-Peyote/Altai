"""Authored grasp families inspired by human power, handle and supporting grasps.
Angles are rig tuning values, not measurements copied from the reference literature.
"""
import unreal
E=unreal.EditorAssetLibrary
root='/Game/Altai/Environment/Geometry/'
def pose(index,middle,ring,little,thumb,opposition=15):
 p=unreal.AltaiFingerGrasp()
 for name,angles in zip(['index','middle','ring','little','thumb'],[index,middle,ring,little,thumb]):p.set_editor_property(name,unreal.Vector(*angles))
 p.set_editor_property('thumb_opposition',opposition);return p
wrap=pose((35,65,35),(40,70,35),(45,75,40),(50,80,40),(15,30,15),20)
bottle=pose((25,50,25),(30,55,30),(35,60,30),(40,65,35),(12,25,12),15)
hook=pose((50,85,45),(55,90,45),(35,55,30),(40,65,35),(10,20,10),10)
rim=pose((40,70,35),(45,75,40),(50,80,40),(55,80,40),(5,15,10),5)
broad=pose((15,35,20),(20,40,25),(25,45,25),(30,50,30),(10,20,12),10)
rod=pose((45,85,45),(50,90,45),(55,90,50),(60,90,50),(20,35,15),25)
for name,label,one,two in [('SM_HandFlask','CylinderSmall',wrap,wrap),('SM_HandBottle','CylinderMedium',bottle,bottle),('SM_HandCup','Handle',hook,broad),('SM_FieldBucket','HandleAndRim',hook,rim),('SM_FieldStick','PowerRod',rod,rod),('SM_FieldStone','Spherical',broad,broad),('SM_FieldCrate','Support',broad,broad)]:
 mesh=unreal.load_asset(root+name);assert mesh,name
 data=list(mesh.get_editor_property('asset_user_data'));profile=next((d for d in data if isinstance(d,unreal.AltaiGripProfile)),None)
 if not profile:profile=unreal.new_object(unreal.AltaiGripProfile,outer=mesh);data.append(profile)
 positive,negative=((55,25,12),(35,25,12)) if 'Cylinder' in label else (((10,8,10),(55,20,12)) if name=='SM_FieldStick' else (((45,20,10),(45,20,8)) if name=='SM_HandCup' else ((35,20,10),(35,20,10))))
 profile.set_editor_property('one_hand_positive_limits',unreal.Vector(*positive));profile.set_editor_property('one_hand_negative_limits',unreal.Vector(*negative));profile.set_editor_property('two_hand_limits',unreal.Vector(20,20,10))
 profile.set_editor_property('carry_height_offset',-22 if name=='SM_FieldBucket' else 0)
 profile.set_editor_property('grasp_name',label);profile.set_editor_property('one_hand',one);profile.set_editor_property('two_hands',two);mesh.set_editor_property('asset_user_data',data)
 # Socket frames specify the wrist and palm, not a point at the centre of the object.
 if name in ['SM_HandFlask','SM_HandBottle']:
  grip=mesh.find_socket('Grip_One');v=grip.get_editor_property('relative_location');v.z=.5 if name.endswith('Flask') else 2;grip.set_editor_property('relative_location',v)
 if name=='SM_HandCup':
  grip=mesh.find_socket('Grip_One');grip.set_editor_property('relative_location',unreal.Vector(0,12,13));grip.set_editor_property('relative_rotation',unreal.MathLibrary.make_rot_from_xz(unreal.Vector(0,0,-1),unreal.Vector(0,-1,0)))
 if name=='SM_FieldBucket':
  for socket_name,y in [('Grip_L',-24),('Grip_R',24)]:
   grip=mesh.find_socket(socket_name);grip.set_editor_property('relative_location',unreal.Vector(0,y,42));grip.set_editor_property('relative_rotation',unreal.MathLibrary.make_rot_from_xz(unreal.Vector(0,0,-1),unreal.Vector(0,1 if y<0 else -1,0)))
  grip=mesh.find_socket('Grip_One');grip.set_editor_property('relative_location',unreal.Vector(5,0,68));grip.set_editor_property('relative_rotation',unreal.MathLibrary.make_rot_from_xz(unreal.Vector(0,0,-1),unreal.Vector(-1,0,0)))
 E.save_loaded_asset(mesh,False)
unreal.log('ALTAI_GRASP_PROFILES_SAVED')
