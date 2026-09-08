"""Import selected CC0 scans; deterministic naming, bounded texture size and mesh LODs."""
import unreal,json
from pathlib import Path
ROOT='/Game/Altai/Environment'
SRC=Path(unreal.Paths.project_dir())/'SourceArt/Environment/PolyHaven'
AT=unreal.AssetToolsHelpers.get_asset_tools();EA=unreal.EditorAssetLibrary;ME=unreal.MaterialEditingLibrary
SM=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
manifest=json.loads((SRC/'manifest.json').read_text())

def save(a): EA.save_loaded_asset(a,only_if_is_dirty=False)
def node(mat,kind,**props):
    n=ME.create_material_expression(mat,getattr(unreal,kind),0,0)
    for k,v in props.items():n.set_editor_property(k,v)
    return n

def link(a,out,b,inp):ME.connect_material_expressions(a,out,b,inp)
def output(n,prop,out=''):ME.connect_material_property(n,out,getattr(unreal.MaterialProperty,prop))
def const(m,v):return node(m,'MaterialExpressionConstant',r=v)
def vector(m,c):return node(m,'MaterialExpressionConstant3Vector',constant=unreal.LinearColor(*c,1))

def tex_import(path,name,folder):
    existing=unreal.load_asset(folder+'/'+name)
    if existing:return existing
    t=unreal.AssetImportTask();t.filename=str(path);t.destination_path=folder;t.destination_name=name;t.automated=True;t.save=True;t.replace_existing=False
    AT.import_asset_tasks([t]);return unreal.load_asset(folder+'/'+name)

collection=unreal.load_asset(ROOT+'/Weather/MPC_Environment')
if not collection:
    collection=AT.create_asset('MPC_Environment',ROOT+'/Weather',unreal.MaterialParameterCollection,unreal.MaterialParameterCollectionFactoryNew())
    params=[]
    for name in ['Wetness','SnowCover']:
        p=unreal.CollectionScalarParameter();p.set_editor_property('parameter_name',name);p.set_editor_property('default_value',0);params.append(p)
    collection.set_editor_property('scalar_parameters',params);save(collection)

materials={};textures={};meshes={}
for entry in manifest:
    ident=entry['id']
    if ident=='fir_tree_01':continue
    folder=ROOT+'/Scans/'+ident;loaded={}
    for key,file in entry['files'].items():
        if key=='mesh':continue
        texture=tex_import(SRC/ident/file,'T_'+ident+'_'+key,folder)
        if not texture:raise RuntimeError('Texture import failed '+ident+'/'+key)
        if 'nor_dx' in key:
            texture.set_editor_property('srgb',False);texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif 'rough' in key.lower() or 'alpha' in key.lower():texture.set_editor_property('srgb',False)
        texture.set_editor_property('max_texture_size',2048 if ident in ['forest_floor','brown_mud_03','rock_face_03'] else 1024)
        save(texture);loaded[key]=texture.get_path_name()
    textures[ident]=loaded
    prefixes=[k[:-4] for k in loaded if k.endswith('diff')]
    if 'Diffuse' in loaded:prefixes=['']
    mats={}
    for prefix in prefixes:
        name='M_'+ident+('_'+prefix.strip('_') if prefix else '')
        m=unreal.load_asset(folder+'/'+name)
        if not m:
            m=AT.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
            dk=prefix+'diff' if prefix else 'Diffuse';nk=prefix+'nor_dx' if prefix else 'nor_dx';rk=prefix+'rough' if prefix else 'Rough';ak=prefix+'alpha' if prefix else 'Alpha'
            d=node(m,'MaterialExpressionTextureSample',texture=unreal.load_asset(loaded[dk]));
            wet=node(m,'MaterialExpressionCollectionParameter',collection=collection,parameter_name='Wetness')
            dark=node(m,'MaterialExpressionLinearInterpolate');link(wet,'',dark,'Alpha');link(const(m,1),'',dark,'A');link(const(m,.6),'',dark,'B')
            mult=node(m,'MaterialExpressionMultiply');link(d,'RGB',mult,'A');link(dark,'',mult,'B')
            output(mult,'MP_BASE_COLOR')
            if nk in loaded:
                n=node(m,'MaterialExpressionTextureSample',texture=unreal.load_asset(loaded[nk]),sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL);output(n,'MP_NORMAL','RGB')
            rough=node(m,'MaterialExpressionLinearInterpolate');link(wet,'',rough,'Alpha');link(const(m,.82),'',rough,'A');link(const(m,.24),'',rough,'B');output(rough,'MP_ROUGHNESS')
            if ak in loaded:
                m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True)
                a=node(m,'MaterialExpressionTextureSample',texture=unreal.load_asset(loaded[ak]),sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR);output(a,'MP_OPACITY_MASK','R')
            ME.recompile_material(m);save(m)
        mats[prefix.strip('_')]=m.get_path_name()
    materials[ident]=mats
    if 'mesh' in entry['files']:
        meshfolder=folder+'/Meshes'
        found=[unreal.load_asset(p) for p in EA.list_assets(meshfolder,recursive=True) if isinstance(unreal.load_asset(p),unreal.StaticMesh)]
        if not found:
            opts=unreal.FbxImportUI();opts.set_editor_property('import_mesh',True);opts.set_editor_property('import_as_skeletal',False)
            opts.set_editor_property('import_materials',False);opts.set_editor_property('import_textures',False);opts.set_editor_property('import_animations',False)
            opts.static_mesh_import_data.set_editor_property('combine_meshes',False)
            opts.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
            t=unreal.AssetImportTask();t.filename=str(SRC/ident/entry['files']['mesh']);t.destination_path=meshfolder;t.automated=True;t.options=opts;t.save=True
            AT.import_asset_tasks([t]);found=[unreal.load_asset(p) for p in t.imported_object_paths if isinstance(unreal.load_asset(p),unreal.StaticMesh)]
        info=[]
        for mesh in found:
            slots=mesh.get_editor_property('static_materials')
            for i,slot in enumerate(slots):
                key=str(slot.material_slot_name).lower();match=next((v for k,v in mats.items() if k and k in key),None) or next(iter(mats.values()))
                mesh.set_material(i,unreal.load_asset(match))
            if not any(s in ident for s in ['tree','sapling','grass']):
                body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            if mesh.get_num_lods()<3:
                settings=unreal.StaticMeshReductionOptions();settings.set_editor_property('auto_compute_lod_screen_size',False)
                values=[]
                for percent,screen in [(1,1),(.35,.35),(.1,.12),(.03,.04)]:
                    s=unreal.StaticMeshReductionSettings();s.set_editor_property('percent_triangles',percent);s.set_editor_property('screen_size',screen);values.append(s)
                settings.set_editor_property('reduction_settings',values);SM.set_lods(mesh,settings)
            save(mesh);b=mesh.get_bounding_box()
            info.append({'path':mesh.get_path_name(),'min':[b.min.x,b.min.y,b.min.z],'max':[b.max.x,b.max.y,b.max.z],'slots':[str(s.material_slot_name) for s in slots]})
        meshes[ident]=info
    unreal.log('ALTAI_IMPORTED '+ident)
report={'textures':textures,'materials':materials,'meshes':meshes}
Path(unreal.Paths.project_saved_dir(),'environment_assets.json').write_text(json.dumps(report,indent=2))
unreal.log('ALTAI_ENVIRONMENT_ASSETS_READY')
