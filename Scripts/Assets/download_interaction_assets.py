from pathlib import Path
import subprocess,json,concurrent.futures,hashlib
root=Path(__file__).resolve().parents[2]/'SourceArt/Environment/InteractionAssets';root.mkdir(parents=True,exist_ok=True)
ids=['treasure_chest','vintage_wooden_drawer_01','large_castle_door'];jobs=[];manifest={}
for ident in ids:
 d=json.loads(subprocess.check_output(['curl','-fsSL','https://api.polyhaven.com/files/'+ident]));folder=root/ident;folder.mkdir(exist_ok=True);(folder/'files.json').write_text(json.dumps(d,indent=2));spec=d['gltf']['2k']['gltf'];jobs.append((folder/(ident+'.gltf'),spec));jobs.extend((folder/name,info) for name,info in spec['include'].items());manifest[ident]={'source':'https://polyhaven.com/a/'+ident,'license':'CC0','format':'glTF 2K'}
def download(job):
 path,info=job;path.parent.mkdir(parents=True,exist_ok=True)
 if not path.exists():subprocess.run(['curl','-fL','--retry','3','-sS',info['url'],'-o',str(path)],check=True)
 assert hashlib.md5(path.read_bytes()).hexdigest()==info['md5'],str(path)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:list(pool.map(download,jobs))
(root/'sources.json').write_text(json.dumps(manifest,indent=2))
for ident in ids:
 d=json.loads((root/ident/(ident+'.gltf')).read_text());print(ident,[(i,n.get('name'),n.get('mesh')) for i,n in enumerate(d['nodes'])])
