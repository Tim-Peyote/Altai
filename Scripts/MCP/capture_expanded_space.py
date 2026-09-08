from mcp import call
from pathlib import Path
import json,time,base64
root=Path(__file__).resolve().parents[2]
def f(n,a={},group='SlateInspectorToolset.SlateInspectorToolset'):
 r=call('call_tool',dict(toolset_name=group,tool_name=n,arguments=a))['result']
 if r.get('isError'):raise RuntimeError(r)
 return json.loads(r['content'][0]['text']).get('returnValue')
for view in ['camp','course','overview']:
 (root/'Saved/expanded_view.json').write_text(json.dumps({'view':view}))
 f('Click',{'ref':'tb1'});f('PressKey',{'key':'Ctrl+A'});f('Type',{'ref':'tb1','text':'py "'+str(root/'Scripts/Editor/view_expanded_space.py')+'"','submit':True});time.sleep(1.5)
 v=f('CaptureEditorImage',{},'EditorToolset.EditorAppToolset');(root/'docs/Images'/('expanded_space_'+view+'.png')).write_bytes(base64.b64decode(v['data']))
print('EXPANDED_SPACE_CAPTURED')
