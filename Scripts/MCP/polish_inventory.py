import sys,json,pathlib,shutil
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent));from mcp_fast import *
BASE='/Game/Altai/UI/'
FONT={'refPath':BASE+'Fonts/F_Display.F_Display'}
def c(r,g=None,b=None,a=1):return {'r':r,'g':r if g is None else g,'b':r if b is None else b,'a':a}
def sc(v):return {'specifiedColor':v,'colorUseRule':'UseColor_Specified'}
def setp(ref,values):
 props=json.loads(tool(OBJ,'list_properties',instance=ref));assert all(k in props for k in values),(ref,list(values),list(props))
 tool(OBJ,'get_properties',instance=ref,properties=list(values))
 assert tool(OBJ,'set_properties',instance=ref,values=json.dumps(values)),ref
class Tree:
 def __init__(self,name):
  self.name=name;self.bp={'refPath':BASE+name+'.'+name};self.w={x['widgetName']:x for x in tool(UMG,'GetWidgets',widgetBlueprint=self.bp)['widgets'] if isinstance(x['widget'],dict)}
 def ref(self,n):return self.w[n]['widget']
 def add(self,cls,n,parent,index=-1):
  if n in self.w:return self.ref(n)
  x=tool(UMG,'AddWidget',widgetBlueprint=self.bp,widgetClass={'refPath':'/Script/UMG.'+cls},widgetDisplayName=n,parentWidget=self.ref(parent),childIndex=index);self.w[n]=x
  tool(UMG,'ToggleWidgetAsVariable',widgetBlueprint=self.bp,widget=x['widget'],bIsVariable=True);return x['widget']
 def text(self,n,parent,value,size=20,serif=False):
  ref=self.add('TextBlock',n,parent);font={'fontObject':FONT if serif else {'refPath':'/Engine/EngineFonts/Roboto.Roboto'},'typefaceFontName':'Regular','size':size}
  setp(ref,{'text':value,'font':font,'colorAndOpacity':sc(c(.72,.68,.58)),'autoWrapText':True});return ref
 def slot(self,n,v):setp(self.w[n]['slot'],v)
 def button(self,n,parent,label,transparent=False):
  ref=self.add('Button',n,parent)
  s=json.loads(tool(OBJ,'get_properties',instance=self.ref('AssignButton') if 'AssignButton' in self.w else ref,properties=['widgetStyle']))['widgetStyle']
  for state in ['normal','hovered','pressed','disabled']:
   s[state]['drawAs']='Image';s[state]['tintColor']=sc(c(.018 if state=='hovered' else .006,a=0 if transparent or state=='normal' else 1))
  s['normalPadding']=s['pressedPadding']={'left':8,'right':8,'top':7,'bottom':7}
  s['normalForeground']=sc(c(.72,.68,.58));s['hoveredForeground']=sc(c(.94,.85,.62));setp(ref,{'widgetStyle':s})
  if label:self.text(n+'Label',n,label,21,True)
  return ref
 def save(self):assert tool(UMG,'CompileWidgetBlueprint',widgetBlueprint=self.bp);assert tool('editor_toolset.toolsets.asset.AssetTools','save_assets',asset_paths=[self.bp['refPath']]);print('saved',self.name,flush=True)
def canvas(x,y,w,h,z=0):return {'layoutData':{'anchors':{'minimum':{'x':x,'y':y},'maximum':{'x':x+w,'y':y+h}},'offsets':{'left':0,'top':0,'right':0,'bottom':0},'alignment':{'x':0,'y':0}},'zOrder':z}
root=pathlib.Path(__file__).resolve().parents[2];backup=root/'Saved/InventoryPolishBackup';backup.mkdir(exist_ok=True)
for p in (root/'Content/Altai/UI').glob('WBP_*.uasset'):
 if not (backup/p.name).exists():shutil.copy2(p,backup/p.name)
# Display typography across all menu screens, with body copy retaining its readable sans-serif face.
for p in (root/'Content/Altai/UI').glob('WBP_*.uasset'):
 if p.stem=='WBP_PreviewSurface':continue
 t=Tree(p.stem)
 for n,x in t.w.items():
  if x['widgetClassPath']['refPath']!='/Script/UMG.TextBlock':continue
  if n in ['Heading','SelectedName','SelectedCategory'] or n.endswith('Label'):
   f=json.loads(tool(OBJ,'get_properties',instance=x['widget'],properties=['font']))['font'];f.update({'fontObject':FONT,'typefaceFontName':'Regular','letterSpacing':35,'size':106 if p.stem=='WBP_MainMenu' and n=='Heading' else 40 if n=='Heading' else 32 if n=='SelectedName' else 25 if n.endswith('Label') else 19})
   setp(x['widget'],{'font':f})
 t.save()
# Compact item cells; four columns are populated by the native view, without a capacity limit.
t=Tree('WBP_InventoryItem');setp(t.ref('IconSize'),{'heightOverride':68,'widthOverride':68,'bOverride_WidthOverride':True,'bOverride_HeightOverride':True})
setp(t.ref('ItemName'),{'font':{'fontObject':{'refPath':'/Engine/EngineFonts/Roboto.Roboto'},'typefaceFontName':'Regular','size':12},'autoWrapText':False,'clipping':'ClipToBounds','textOverflowPolicy':'Ellipsis'})
t.save()
# Equipment gets the actual item icon and quantity.
t=Tree('WBP_EquipmentCell');t.add('SizeBox','EquipmentIconSize','Content',1);setp(t.ref('EquipmentIconSize'),{'heightOverride':54,'widthOverride':54,'bOverride_WidthOverride':True,'bOverride_HeightOverride':True});t.slot('EquipmentIconSize',{'horizontalAlignment':'HAlign_Center'})
t.add('Image','EquippedIcon','EquipmentIconSize');t.text('EquippedQuantity','Content','',11)
setp(t.ref('SlotName'),{'font':{'fontObject':FONT,'typefaceFontName':'Regular','size':17},'autoWrapText':False});t.save()
t=Tree('WBP_FieldInventory')
t.add('HorizontalBox','SortRow','BagContent',1);t.button('SortButton','SortRow','');t.text('SortLabel','SortButton','Сортировка: тип',18,True);t.slot('SortRow',{'padding':{'left':0,'top':4,'right':0,'bottom':8}})
setp(t.ref('ItemGrid'),{'minDesiredSlotHeight':116,'minDesiredSlotWidth':76,'slotPadding':{'left':3,'top':3,'right':3,'bottom':3}})
t.text('InteractionHint','DescriptionContent','Перетащите в ячейку • ПКМ — действия',13)
for n,(x,y) in {'HeadSlot':(.655,.16),'BodySlot':(.345,.42),'HandSlot':(.655,.51),'QuickSlot1':(.44,.80),'QuickSlot2':(.545,.80)}.items():t.slot(n,canvas(x,y,.08,.16,4))
if 'InventoryTabBar' in t.w:t.slot('InventoryTabBar',canvas(.045,.025,.39,.055,2))
setp(t.ref('SelectedIconSize'),{'widthOverride':154,'heightOverride':154,'bOverride_WidthOverride':True,'bOverride_HeightOverride':True})
for n in ['Help','PreviewHint','ControlsHint','FooterHint']:
 if n in t.w and t.w[n]['widgetClassPath']['refPath']=='/Script/UMG.TextBlock':setp(t.ref(n),{'text':'ЛКМ — выбрать / тянуть    •    ПКМ — действия    •    R — сортировка'})
# Popup: a real editable widget subtree, not a generated runtime layout.
t.button('ContextDismiss','Root','',True);t.slot('ContextDismiss',canvas(0,0,1,1,100));setp(t.ref('ContextDismiss'),{'visibility':'Collapsed','isFocusable':False})
t.add('Border','ContextPanel','Root');setp(t.ref('ContextPanel'),{'brushColor':c(.012,.011,.009),'padding':{'left':16,'top':14,'right':16,'bottom':12},'visibility':'Collapsed'})
t.slot('ContextPanel',{'layoutData':{'anchors':{'minimum':{'x':0,'y':0},'maximum':{'x':0,'y':0}},'offsets':{'left':300,'top':200,'right':280,'bottom':300},'alignment':{'x':0,'y':0}},'bAutoSize':True,'zOrder':101})
t.add('SizeBox','ContextWidth','ContextPanel');setp(t.ref('ContextWidth'),{'widthOverride':248,'bOverride_WidthOverride':True})
t.add('VerticalBox','ContextContent','ContextWidth');t.text('ContextTitle','ContextContent','Предмет',26,True)
t.text('ContextEmpty','ContextContent','Нет доступных действий для этого предмета.',14)
for n,label in [('ContextEquip','Надеть / взять'),('ContextQuick1','В быстрый слот 1'),('ContextQuick2','В быстрый слот 2'),('ContextRemove','Снять назначение'),('ContextClose','Закрыть')]:t.button(n,'ContextContent',label)
t.save()

# Stable single-line action labels and sort-row sizing.
t=Tree('WBP_FieldInventory')
for n,x in t.w.items():
 if x['widgetClassPath']['refPath']=='/Script/UMG.TextBlock' and n.endswith('Label'):
  setp(x['widget'],{'autoWrapText':False})
  if n in ['AssignButtonLabel','UnassignButtonLabel']:
   f=json.loads(tool(OBJ,'get_properties',instance=x['widget'],properties=['font']))['font'];f['size']=22;setp(x['widget'],{'font':f})
t.slot('SortButton',{'size':{'sizeRule':'Fill','value':1},'horizontalAlignment':'HAlign_Fill'})
t.slot('SortLabel',{'horizontalAlignment':'HAlign_Left'})
t.save()
