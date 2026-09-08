import unreal
p=unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class('/Game/Altai/Player/BP_AltaiCharacter'))
mesh=p.mesh.get_skeletal_mesh_asset()
if not unreal.AltaiEditorLibrary.create_contact_animation(mesh.get_editor_property('skeleton')):
 raise RuntimeError('Contact animation graph failed')
unreal.log('ALTAI_CONTACT_GRAPH_CREATED')
