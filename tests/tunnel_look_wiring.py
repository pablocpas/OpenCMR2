import re,sys
h=open('src/render/tunnel_vision.h').read(); sh=open('src/render/shaders/dof.frag').read()
g=open('src/render/gfx_gpu.cpp').read(); e=open('src/render/gl_effects_executor.h').read(); gl=open('src/render/gfx_gl.cpp').read()
ok=True
def need(c,m):
    global ok
    if not c: print("FAIL",m); ok=False
rad=float(re.search(r'kTunnelRadiusPerHeight = ([0-9.]+)f',h).group(1)); inner=float(re.search(r'kTunnelInnerFull = ([0-9.]+)f',h).group(1))
m=re.search(r'float inner = max\(mix\(1\.4, ([0-9.]+), tunnel\), ([0-9.]+)\);',sh)
need(m is not None and abs(float(m.group(1))-inner)<1e-6,'dof.frag inner radius mirrors kTunnelInnerFull')
imin=float(re.search(r'kTunnelInnerMin = ([0-9.]+)f',h).group(1))
need(m is not None and abs(float(m.group(2))-imin)<1e-6,'dof.frag inner floor mirrors kTunnelInnerMin')
need('kTunnelMaxStrength = 2.0f' in h and 'kTunnelMaxStrength' in g and 'kTunnelMaxStrength' in gl,'overdrive cap shared by both backends')
vs=float(re.search(r'kTunnelVerticalScale = ([0-9.]+)f',h).group(1))
need('centered.y *= %s;'%re.search(r'centered\.y \*= ([0-9.]+);',sh).group(1) in sh and abs(float(re.search(r'centered\.y \*= ([0-9.]+);',sh).group(1))-vs)<1e-6,'dof.frag vertical scale mirrors header')
import math
def ss(a,b,x):
    t=min(1,max(0,(x-a)/(b-a))); return t*t*(3-2*t)
def periph(x,y,asp=1.54,t=1.0):  # x,y in -1..1 screen coords, strength t
    inn=1.4+(inner-1.4)*t; return ss(inn,inn+0.8,math.hypot(x*asp,y*vs))*t
need(periph(0,-1.0)==0 and periph(0,-0.9)==0,'bonnet, bottom centre, stays sharp at full strength')
need(periph(0,-0.46)==0 and periph(0.3,-0.46)<0.02,'chase car (about 0.46 below centre, off-centre by 0.3) stays sharp')
need(periph(0.2,-0.9)<0.02,'bonnet within 0.2 of centre stays sharp (wider than that softens by design: a symmetric clear zone cannot also blur the sides)')
need(periph(0.95,0)>0.6 and periph(0.75,0)>0.25,'side periphery blurs clearly')
need(periph(0,0)==0,'centre sharp')
need(0.02<=rad<=0.04,'radius stays in the range where 16 taps are not sparse')
need(0.3<=inner<=0.7,'clear circle keeps a car in a chase view (about 0.5 half-heights below centre) sharp')
need('kTunnelRadiusPerHeight * (float)backHeight' in g,'SDL_GPU uses the shared radius')
need('kTunnelRadiusPerHeight * (float)backHeight' in e,'GL uses the shared radius')
need('render/tunnel_vision.h' in g and 'render/tunnel_vision.h' in gl,'both backends include it')
need('0.014f' not in g and '0.014f' not in e,'no stale radius literal')
r=open('game/Race.cpp').read()
need('Sys_GetOption("video.cockpit_blur_clear", 0)' in r,'cockpit interior depth exemption is opt-in (default 0): it covered ~99 percent of the blur zone')
sys.exit(0 if ok else 1)
