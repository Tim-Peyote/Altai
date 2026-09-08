# Authoring only: applies the shared UI palette to editable UMG assets via Unreal MCP.
import sys,json,copy,pathlib,shutil
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from mcp_helpers import *
ROOT=pathlib.Path(__file__).resolve().parents[2]
backup=ROOT/'Saved/UIThemeBackup';backup.mkdir(parents=True,exist_ok=True)
PALETTE=json.loads((ROOT/'SourceArt/UI/palette.json').read_text())
INK=PALETTE['background'];BONE=PALETTE['text'];MUTED=PALETTE['muted'];GOLD=PALETTE['accent']
def color(c):return {'specifiedColor':c,'colorUseRule':'UseColor_Specified'}
def rgba(v,a=1):return {'r':v,'g':v,'b':v,'a':a}
def props(ref):return json.loads(tool(OBJ,'list_properties',instance=ref))
def get(ref,keys):return json.loads(tool(OBJ,'get_properties',instance=ref,properties=keys))
def setp(ref,values):assert tool(OBJ,'set_properties',instance=ref,values=json.dumps(values)),(ref,values)
if tool('EditorToolset.EditorAppToolset','IsPIERunning'):
 raise RuntimeError('Stop PIE before applying the UI theme.')
assets=[]
for path in sorted((ROOT/'Content/Altai/UI').glob('WBP_*.uasset')):
 name=path.stem
 if name in ['WBP_PreviewSurface']:continue
 if not (backup/path.name).exists():shutil.copy2(path,backup/path.name)
 bp={'refPath':f'/Game/Altai/UI/{name}.{name}'}
 w=tool(UMG,'GetWidgets',widgetBlueprint=bp)['widgets'];main=name=='WBP_MainMenu';field=name in ['WBP_FieldInventory','WBP_FieldMap'];cell=name in ['WBP_InventoryItem','WBP_EquipmentCell'];profile=name=='WBP_ProfileRow'
 for x in w:
  if not isinstance(x['widget'],dict):continue
  ref=x['widget'];cls=x['widgetClassPath']['refPath'].split('.')[-1];n=x['widgetName'];p=props(ref)
  if 'toolTipText' in p and name in ['WBP_FieldInventory','WBP_EquipmentCell','WBP_InventoryItem']:setp(ref,{'toolTipText':''})
  if cls=='Border' and n!='TitleRule':
   get(ref,['brushColor']);setp(ref,{'brushColor':INK})
  elif cls=='Button':
   d=get(ref,['widgetStyle','backgroundColor']);s=d['widgetStyle'];is_cell=cell or profile;is_tab=n in ['InventoryTab','MapTab','FilterAll','FilterMaterials','FilterEquipment','FilterTools'];
   for state,v,edge in [('normal',.007,.075),('hovered',.026,.28),('pressed',.019,.40),('disabled',.003,.027)]:
    b=s[state];b['drawAs']='RoundedBox';b['resourceObject']='None';b['tintColor']=color(rgba(v,1 if is_cell or state!='normal' else 0));b['outlineSettings'].update({'cornerRadii':{'x':0,'y':0,'z':0,'w':0},'roundingType':'FixedRadius','width':.7 if is_cell else 0,'color':color(rgba(edge)),'bUseBrushTransparency':False})
   if is_tab:
    s['normal']['tintColor']=color(rgba(.018))
    s['normal']['outlineSettings']['width']=.65
    s['normal']['outlineSettings']['color']=color({'r':.13,'g':.12,'b':.1,'a':1})
   for state,c in [('normalForeground',BONE),('hoveredForeground',{'r':.95,'g':.9,'b':.76,'a':1}),('pressedForeground',GOLD),('disabledForeground',MUTED)]:s[state]=color(c)
   if main:
    s['hovered']['tintColor']=color({'r':.006,'g':.005,'b':.003,'a':1})
    s['hoveredForeground']=color({'r':.8,'g':.68,'b':.46,'a':1})
   pad={'left':12 if is_cell else 0,'right':12 if is_cell else 0,'top':8,'bottom':8};s['normalPadding']=pad;s['pressedPadding']=pad
   setp(ref,{'widgetStyle':s,'backgroundColor':rgba(1)})
  elif cls=='TextBlock':
   d=get(ref,['font','colorAndOpacity','text']);f=d['font'];heading=n in ['Heading','SelectedName'];label=n.endswith('Label') or n in ['AssignLabel'];muted=n in ['Eyebrow','StatusText','Hint','SlotName','SelectedCategory','ItemQuantity','ProfileDetails']
   f['typefaceFontName']='Light' if heading else 'Regular';f['letterSpacing']=130 if main and heading else 70 if heading else 90 if n=='Eyebrow' else 45 if label else 0
   if heading:f['size']=86 if main else 34 if n=='Heading' else 26
   elif main and label:f['size']=21
   elif n=='Eyebrow':f['size']=11
   elif n=='SlotName':f['size']=12
   elif n=='ItemName':f['size']=14
   elif n=='ItemQuantity':f['size']=12
   c=MUTED if muted else BONE
   values={'font':f,'colorAndOpacity':color(c)}
   if n=='SlotName':values['autoWrapText']=False
   if label:values['colorAndOpacity']={'specifiedColor':BONE,'colorUseRule':'UseColor_Foreground'}
   if main and n=='Eyebrow':values['text']='Э К С П Е Д И Ц И Я'
   if label and not field and not cell and not profile:
    values['justification']='Left'
    if isinstance(x['slot'],dict):
     sp=props(x['slot'])
     if 'horizontalAlignment' in sp:
      get(x['slot'],['horizontalAlignment']);setp(x['slot'],{'horizontalAlignment':'HAlign_Left'})
   setp(ref,values)
  elif cls=='EditableTextBox':
   d=get(ref,['widgetStyle']);s=d['widgetStyle']
   for key in ['backgroundImageNormal','backgroundImageHovered','backgroundImageFocused','backgroundImageReadOnly']:
    if key in s:
     b=s[key];b['drawAs']='RoundedBox';b['tintColor']=color(rgba(.01));b['outlineSettings'].update({'width':.6,'cornerRadii':{'x':0,'y':0,'z':0,'w':0},'roundingType':'FixedRadius','color':color(GOLD if key.endswith('Focused') else rgba(.1))})
   for key in ['foregroundColor','focusedForegroundColor']:s[key]=color(BONE)
   setp(ref,{'widgetStyle':s})
  if n=='Content' and cls=='VerticalBox' and not cell and not profile and not field:
   sp=props(x['slot'])
   if 'layoutData' in sp:
    d=get(x['slot'],['layoutData']);v=d['layoutData'];v['anchors']={'minimum':{'x':.105,'y':.20 if main else .17},'maximum':{'x':.47 if main else .56,'y':.93}};setp(x['slot'],{'layoutData':v})
  if cls=='SizeBox' and main:
   if 'minDesiredHeight' in p:setp(ref,{'minDesiredHeight':48})
 assert tool(UMG,'CompileWidgetBlueprint',widgetBlueprint=bp),name
 assets.append('/Game/Altai/UI/'+name+'.'+name)
 print('styled',name,flush=True)
assert tool('editor_toolset.toolsets.asset.AssetTools','save_assets',asset_paths=assets)
print('saved',len(assets))
