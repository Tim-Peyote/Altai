from mcp import call
from pathlib import Path
import json,base64
args={'toolset_name':'EditorToolset.EditorAppToolset','tool_name':'CaptureViewport','arguments':{'captureTransform':{'location':{'x':5100,'y':-2200,'z':350},'rotation':{'pitch':1,'yaw':150,'roll':0}},'annotations':None,'bShowUI':False}}
r=call('call_tool',args)['result'];v=json.loads(r['content'][0]['text'])['returnValue']['image']
p=Path(__file__).resolve().parents[2]/'docs/Images/character_lab.png';p.parent.mkdir(exist_ok=True);p.write_bytes(base64.b64decode(v['data']));print(p)
