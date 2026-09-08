import unreal
E=unreal.EditorAssetLibrary;SM=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
for name,center,width in [('SM_PH_DoorLeft',37,74),('SM_PH_DoorRight',-37.5,75)]:
 m=unreal.load_asset('/Game/Altai/Environment/Interaction/'+name);SM.remove_collisions(m);body=m.get_editor_property('body_setup');b=unreal.KBoxElem();b.set_editor_property('center',unreal.Vector(0,center,111));b.set_editor_property('x',8);b.set_editor_property('y',width);b.set_editor_property('z',222);agg=body.get_editor_property('agg_geom');agg.set_editor_property('box_elems',[b]);body.set_editor_property('agg_geom',agg);E.save_loaded_asset(m,only_if_is_dirty=False)
unreal.log('FURNITURE_DOOR_COLLIDERS_REFINED')
