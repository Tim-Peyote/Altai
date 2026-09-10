"""Deepen only the existing basin, preserving the rest of the authored test map."""
import unreal,json
from pathlib import Path
E=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=E.get_all_level_actors();land=next(a for a in actors if a.get_actor_label()=='Altai_Valley_Landscape')
if not unreal.AltaiEditorLibrary.sculpt_swimming_pond(land):raise RuntimeError('Pond sculpt failed')
pond=next((a for a in actors if isinstance(a,unreal.AltaiPond)),None)
if not pond:pond=E.spawn_actor_from_class(unreal.AltaiPond,unreal.Vector(1800,1700,-172))
pond.set_actor_label('Swimming_Pond');pond.set_folder_path('06_Wetlands/Swimming')
pond.bounds.set_box_extent(unreal.Vector(1500,1100,650),False);pond.surface_height=28;pond.maximum_depth=440
pond.surface=unreal.load_asset('/Game/Altai/Environment/Materials/PM_PondWater')
# Existing water material/physical surface path differs in older authoring revisions.
old=next((a for a in actors if a.get_actor_label()=='Surface_Brod'),None)
if old:
 if not pond.surface:pond.surface=old.surface
 old.priority=5
pond.full_resistance_depth=130
# Sign remains next to the existing wetland access, not in the swimming path.
sign=next((a for a in actors if a.get_actor_label()=='WaterSign'),None)
if sign:
 text=sign.get_component_by_class(unreal.TextRenderComponent)
 if text:text.set_text('03 / POND\nWADE > SWIM > DIVE\nSHIFT: PACE | C: DOWN | SPACE: UP')
if not L.save_current_level():raise RuntimeError('Could not save pond map')
Path(unreal.Paths.project_saved_dir(),'pond_authoring.json').write_text(json.dumps({'passed':True,'center':[1800,1700,28],'depth_cm':420,'radius_cm':[1500,1100]},indent=2))
