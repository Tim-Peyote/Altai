import json
from mcp import request,call
from mcp_helpers import UMG,OBJ,BP,describe
_sid=None
def tool(ts,tool_name,**args):
 global _sid
 if _sid is None:
  _,_sid=request('initialize',{'protocolVersion':'2025-03-26','capabilities':{},'clientInfo':{'name':'AltaiUIPolish','version':'1.0'}})
 r,_=request('tools/call',{'name':'call_tool','arguments':{'toolset_name':ts,'tool_name':tool_name,'arguments':args}},_sid)
 if 'error' in r or r.get('result',{}).get('isError'):raise RuntimeError(r)
 j=json.loads(r['result']['content'][0]['text']);return j.get('returnValue',j)
