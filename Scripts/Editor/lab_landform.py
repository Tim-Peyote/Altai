"""Deterministic editor-only landform. UE Landscape remains sculptable afterwards."""
import math

def radius(x,y,cx,cy,rx,ry):
    return math.sqrt(((x-cx)/rx)**2+((y-cy)/ry)**2)

def height(x,y):
    z=65+28*math.sin(x/690)*math.cos(y/780)+14*math.sin(x/220+y/370)
    z+=2150*math.exp(-((x+4300)/1800)**2-((y-2900)/2300)**2)
    z+=max(0,(abs(x)-5100)/2100)**2*1050
    z+=max(0,(abs(y)-5300)/2000)**2*800
    for cx,cy,rx,ry,low in [(1800,1700,1500,1100,-65),(-700,1300,1050,850,4)]:
        r=radius(x,y,cx,cy,rx,ry)
        if r<1.35:
            blend=max(0,min(1,(1.35-r)/.35))
            target=low+80*min(r,1.35)**2
            z=z*(1-blend)+target*blend
    return z
