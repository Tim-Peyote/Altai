import json
from mcp import call
UMG='UMGToolSet.UMGToolSet';OBJ='editor_toolset.toolsets.object.ObjectTools';BP={'refPath':'/Game/Altai/UI/WBP_FieldInventory.WBP_FieldInventory'}
def tool(ts,name,**args):
 r=call('call_tool',{'toolset_name':ts,'tool_name':name,'arguments':args})
 if 'error' in r or r.get('result',{}).get('isError'):raise RuntimeError(r)
 c=r['result']['content'];j=json.loads(c[0]['text']);return j.get('returnValue',j)
def describe(ts):
 r=call('describe_toolset',{'toolset_name':ts});j=json.loads(r['result']['content'][0]['text']);return j
