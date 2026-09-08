import unreal,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from lab_landform import height
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors={a.get_actor_label():a for a in A.get_all_level_actors()}
trunk=actors['Mature_Fir_14_Trunk'];base=trunk.get_actor_location()
if base.x<5800:
 offset=unreal.Vector(500,0,height(base.x+500,base.y)-height(base.x,base.y))
 for name in ['Mature_Fir_14','Mature_Fir_14_Trunk']:
  a=actors[name];a.set_actor_location(a.get_actor_location()+offset,False,True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level();unreal.log('BEAM_SIGHTLINE_CLEARED')
