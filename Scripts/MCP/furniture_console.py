"""Run an editor helper through the visible console (no reflected hot reload)."""
import sys,time,json
from pathlib import Path
from mcp import call
ROOT=Path(__file__).resolve().parents[2]
def f(tool,args={},group='SlateInspectorToolset.SlateInspectorToolset'):
 r=call('call_tool',dict(toolset_name=group,tool_name=tool,arguments=args))['result']
 if r.get('isError'):raise RuntimeError(r)
 text=r['content'][0]['text']
 try:return json.loads(text).get('returnValue')
 except ValueError:raise RuntimeError(text)
def run(name):
 f('Snapshot',{'ref':'','maxDepth':40});f('Click',{'ref':'tb1'});f('PressKey',{'key':'Ctrl+A'});f('Type',{'ref':'tb1','text':'py "'+str(ROOT/'Scripts/Editor'/name)+'"','submit':True})
if __name__=='__main__':run(sys.argv[1]);print('QUEUED',sys.argv[1])
