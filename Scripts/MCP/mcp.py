import urllib.request,json,sys,os
URL=os.environ.get('ALTAI_MCP_URL','http://127.0.0.1:8000/mcp')
def request(method,params,session=None):
 h={'Content-Type':'application/json','Accept':'application/json, text/event-stream'}
 if session:h['Mcp-Session-Id']=session
 req=urllib.request.Request(URL,data=json.dumps({'jsonrpc':'2.0','id':1,'method':method,'params':params}).encode(),headers=h)
 try:
  with urllib.request.urlopen(req,timeout=180) as r:
   raw=r.read().decode(); sid=r.headers.get('Mcp-Session-Id')
   if raw.startswith('event:') or raw.startswith('data:'):raw=next(l[5:].strip() for l in raw.splitlines() if l.startswith('data:'))
   return json.loads(raw),sid
 except urllib.error.HTTPError as e: raise RuntimeError(e.read().decode())
def call(name,args):
 _,sid=request('initialize',{'protocolVersion':'2025-03-26','capabilities':{},'clientInfo':{'name':'AltaiEditor','version':'1.0'}})
 result,_=request('tools/call',{'name':name,'arguments':args},sid)
 return result
if __name__=='__main__':
 if len(sys.argv)>1: print(json.dumps(call(sys.argv[1],json.loads(sys.argv[2])),ensure_ascii=False))
 else:
  _,sid=request('initialize',{'protocolVersion':'2025-03-26','capabilities':{},'clientInfo':{'name':'AltaiEditor','version':'1.0'}})
  print(json.dumps(request('tools/list',{},sid)[0]))
