import re
def read(p): return open(p).read()
wd=read('game/WindshieldDirt.cpp'); wg=read('game/WindshieldGlass.cpp'); en=read('src/platform/enhancements.h')
assert 'WindshieldGlass_DrawCockpitMesh(mview)' in wd, 'cockpit mesh must draw in the cockpit 3D pass'
for k in ('video.cockpit_mesh','video.dump_interior'):
    assert k in wg and k in en, k
for k in ('--cockpit-mesh','--dump-interior'):
    assert en.count(k)>=3, k+' needs the whitelist, parser and help entries'
assert 'interior_dump.obj' in wg and 'cockpit_mesh.obj' in wg
assert 'WindshieldGlass_DrawCockpitMesh' in read('game/Graphics.h') and 'WindshieldGlass_DrawCockpitMesh' in read('game/GraphicsOptions.h')
print('ok cockpit mesh wiring')
