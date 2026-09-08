"""Original, deterministic low-poly test props and cliff shell; centimetres, Z up."""
import math,random
from pathlib import Path
class Mesh:
 def __init__(self):self.v=[];self.f=[];self.uv=[]
 def face(self,points,uv=None):
  n=len(self.v);self.v.extend(points);self.uv.extend(uv or [(p[1]/100,p[2]/100) for p in points]);self.f.extend([(n,n+i,n+i+1) for i in range(1,len(points)-1)])
 def box(self,c,e):
  x,y,z=c;a,b,d=e
  v=[(x+sx*a,y+sy*b,z+sz*d) for sx,sy,sz in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
  for f in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:self.face([v[i] for i in f],[(0,0),(1,0),(1,1),(0,1)])
 def tube(self,points,radius,n=10):
  rings=[]
  for i,p in enumerate(points):
   q=points[min(i+1,len(points)-1)] if i<len(points)-1 else points[i-1];t=[q[j]-p[j] for j in range(3)];l=math.sqrt(sum(a*a for a in t));t=[a/l for a in t];a=(1,0,0) if abs(t[0])<.9 else (0,1,0)
   b=(t[1]*a[2]-t[2]*a[1],t[2]*a[0]-t[0]*a[2],t[0]*a[1]-t[1]*a[0]);l=math.sqrt(sum(v*v for v in b));b=[v/l for v in b];a=(b[1]*t[2]-b[2]*t[1],b[2]*t[0]-b[0]*t[2],b[0]*t[1]-b[1]*t[0])
   if i==len(points)-1:a=last_a;b=last_b
   last_a=a;last_b=b
   rings.append([tuple(p[k]+radius*(a[k]*math.cos(j/n*math.tau)+b[k]*math.sin(j/n*math.tau)) for k in range(3)) for j in range(n)])
  for i in range(len(rings)-1):
   for j in range(n):self.face([rings[i][j],rings[i][(j+1)%n],rings[i+1][(j+1)%n],rings[i+1][j]])
  self.face(list(reversed(rings[0])));self.face(rings[-1])
 def write(self,path):
  path.write_text('\n'.join(['o '+path.stem]+['v %.6f %.6f %.6f'%v for v in self.v]+['vt %.6f %.6f'%v for v in self.uv]+['s off']+['f '+' '.join(f'{i+1}/{i+1}' for i in f) for f in self.f]))
def make_all(folder):
 folder=Path(folder);folder.mkdir(parents=True,exist_ok=True)
 bucket=Mesh();n=48
 for i in range(n):
  a=i/n*math.tau;b=(i+1)/n*math.tau
  point=lambda r,z,t:(r*math.cos(t),r*math.sin(t),z)
  for r0,z0,r1,z1,reverse in [(14,0,19,34,False),(17.7,34,13,2,False),(19,34,17.7,34,False),(13,2,0,2,False),(0,0,14,0,False)]:bucket.face([point(r0,z0,a),point(r0,z0,b),point(r1,z1,b),point(r1,z1,a)],[(i/n,0),((i+1)/n,0),((i+1)/n,1),(i/n,1)])
 handle=[(0,20*math.cos(i/24*math.pi),34+26*math.sin(i/24*math.pi)) for i in range(25)];bucket.tube(handle,1.05,8)
 bucket.write(folder/'SM_FieldBucket.obj')
 stick=Mesh();stick.tube([(0,-65,3),(-1,-25,1),(1,20,2),(0,65,4)],2.7,9);stick.tube([(0,-20,2),(1,-30,9),(0,-34,12)],1.2,7);stick.write(folder/'SM_FieldStick.obj')
 crate=Mesh()
 for y in [-19,19]:
  for z in [5,15,25,35]:crate.box((0,y,z),(24,1.6,4.4))
 for x in [-23,23]:
  for z in [5,15,25,35]:crate.box((x,0,z),(1.6,18,4.4))
 for y in [-15,-5,5,15]:crate.box((0,y,1),(24,4.5,1.5));crate.box((0,y,39),(24,4.5,1.5))
 for x in [-18,18]:crate.box((x,-21,20),(2,1,20));crate.box((x,21,20),(2,1,20))
 crate.write(folder/'SM_FieldCrate.obj')
 stone=Mesh();rng=random.Random(17);rings=[]
 for i in range(9):
  th=math.pi*i/8;ring=[]
  for j in range(16):
   a=j/16*math.tau;r=1+rng.uniform(-.10,.10);ring.append((15*math.sin(th)*math.cos(a)*r,14*math.sin(th)*math.sin(a)*r,11*math.cos(th)*(1 if i in (0,8) else r)))
  rings.append(ring)
 for i in range(8):
  for j in range(16):
   k=(j+1)%16;points=[rings[i][j],rings[i+1][j],rings[i+1][k],rings[i][k]]
   if i==0:points=points[:3]
   elif i==7:points=[points[0],points[1],points[3]]
   stone.face(points)
 stone.write(folder/'SM_FieldStone.obj')
 # Face relief stays within 2cm of the stable collision plane; the silhouette is irregular.
 rock=Mesh();ys=[-1900,-1750,-1650,-1400,-1200,-1000,-750,-600,-450,-250,0,250,450,600,750,1000,1200,1400,1650,1750,1900];tops=[1840+100*math.sin(y/530)+40*math.cos(y/180) for y in ys]
 # OBJ import reverses Y. Keep the three playable lips at their collision heights.
 from lab_landform import height
 for j,y in enumerate(ys):
  if abs(y)<=450:lane=-4300
  elif -1650<=y<=-750:lane=-3100
  elif 750<=y<=1650:lane=-5500
  else:continue
  tops[j]=height(3500,lane)-30+1600-35
 grid=[]
 for j,y in enumerate(ys):grid.append([((180 if abs(y)==600 else -160 if abs(y)>1650 else 0)+1.6*math.sin(j*3+i*2),y,-100+(tops[j]+100)*i/12) for i in range(13)])
 for j in range(20):
  for i in range(12):rock.face([grid[j][i],grid[j+1][i],grid[j+1][i+1],grid[j][i+1]])
  rock.face([grid[j][-1],grid[j+1][-1],(-400,ys[j+1],tops[j+1]),(-400,ys[j],tops[j])])
  rock.face([(-400,ys[j],tops[j]),(-400,ys[j+1],tops[j+1]),(-600,ys[j+1],tops[j+1]-120),(-600,ys[j],tops[j]-120)])
  rock.face([(-600,ys[j],-150),(-600,ys[j],tops[j]-120),(-600,ys[j+1],tops[j+1]-120),(-600,ys[j+1],-150)])
 rock.face([grid[0][0],grid[0][-1],(-600,ys[0],tops[0]-120),(-600,ys[0],-150)])
 rock.face([grid[-1][-1],grid[-1][0],(-600,ys[-1],-150),(-600,ys[-1],tops[-1]-120)])
 rock.write(folder/'SM_ClimbingEscarpment.obj')
if __name__=='__main__':make_all(Path(__file__).resolve().parents[2]/'SourceArt/Environment/FieldTests')
