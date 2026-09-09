"""Original compact test meshes, in centimetres; no external asset dependencies."""
from pathlib import Path
import math
from lab_test_geometry import Mesh

def lathe(mesh,profile,n=32):
 for a,b in zip(profile,profile[1:]):
  for i in range(n):
   u=i/n*math.tau;v=(i+1)/n*math.tau
   z0,r0=a;z1,r1=b
   mesh.face([(r0*math.cos(u),r0*math.sin(u),z0),(r0*math.cos(v),r0*math.sin(v),z0),(r1*math.cos(v),r1*math.sin(v),z1),(r1*math.cos(u),r1*math.sin(u),z1)],[(i/n,z0/24),((i+1)/n,z0/24),((i+1)/n,z1/24),(i/n,z1/24)])

def make(folder):
 folder=Path(folder);folder.mkdir(parents=True,exist_ok=True)
 for name,profile in [
  ('SM_HandFlask',[(0,0),(0,3.2),(.7,3.7),(9,3.7),(12,2),(16,1.6),(16.5,2),(18,2),(18,0)]),
  ('SM_HandBottle',[(0,0),(0,4.5),(1,5),(14,5),(18,2.5),(23,2.2),(24,2.7),(25,2.7),(25,0)]),
  ('SM_HandCup',[(0,0),(0,4.7),(1,5),(10,5.5),(10,4.7),(1.2,4.2),(1.2,0)])]:
  m=Mesh();lathe(m,profile)
  if name.endswith('Cup'):
   m.tube([(0,-5,8.5),(0,-8,8.5),(0,-9.5,6.5),(0,-9.5,4),(0,-8,2.5),(0,-5,2.5)],.65,8)
  m.write(folder/(name+'.obj'))

if __name__=='__main__':make(Path(__file__).resolve().parents[2]/'SourceArt/Environment/HandTests')
