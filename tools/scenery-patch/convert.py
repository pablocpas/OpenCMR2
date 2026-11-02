# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
from PIL import Image,ImageDraw
import numpy as np,math,json


# Full-canvas source-fit contract. No restored artwork is bundled or fetched.
TREE_CANDIDATES={('frahit.bfl','tre_md1'),('frahit.bfl','tre_md4'),('finhit.bfl','tre_fir1')}
# Additional fixed-cutout mappings reviewed at four angles and against actual
# Finland1/France1 full-canvas cards. Shell defaults stay unchanged.
CROSSED_CANDIDATES={(archive,name) for archive,names in (
 ('finhit.bfl',('tre_fir2','tre_fr3')),('finlot.bfl',('tre_fir2','tre_fr3')),
 ('frahit.bfl',('tre_md2',)),('fralot.bfl',('tre_md2',))) for name in names}
root=None
def crown_profile(alpha):
 # Species-independent crown/stem split from the alpha mask alone. A row is crown while it is at least 22% as wide as the widest row;
 # everything below the last such row is stem. Returns (crown_bottom fraction, last opaque row).
 h=alpha.shape[0];widths=(alpha>=128).sum(axis=1);opaque=np.where(widths>0)[0]
 if len(opaque)<4:raise ValueError('Empty source alpha')
 wide=np.where(widths>=max(2,0.22*widths.max()))[0]
 return float(min(.95,max(.45,(wide.max()+1)/h))),int(opaque.max())

def is_sparse(alpha):
 # Sparse crowns (conifers, thin birches): less than half of the crown's bounding box is opaque. A smooth shell would be mostly holes,
 # so these use the crossed-plane form, which keeps the source branches intact.
 cb,_=crown_profile(alpha);crown=alpha[:round(alpha.shape[0]*cb)]>=128
 cols=np.where(crown.any(0))[0];ys=np.where(crown.any(1))[0]
 if len(cols)<2 or len(ys)<2:return True
 return crown.sum()/((cols.max()-cols.min()+1)*(ys.max()-ys.min()+1))<.50

class NoStemData(ValueError):
 pass

def tree(a,rel,adaptive=False,stem=True):
 name=Path(a['texture']).stem;src=Image.open(a['path']).convert('RGBA');a=np.array(src);h,w=a.shape[:2];p=root/rel;p.mkdir(parents=True,exist_ok=True)
 # Source projection, not circumference repetition. Source alpha is retained.
 crown_bottom=.74 if name.startswith('tre_md') else .88
 last_row=round(h*.94)
 if adaptive:crown_bottom,last_row=crown_profile(a[:,:,3])
 cut=round(h*crown_bottom)
 # Opaque stem-only texels; no nearest-fill transparency or ground colors.
 bark=[]
 bark_rows=range(min(cut+2,last_row-2),max(cut+3,last_row-1)) if adaptive else range(round(h*.78),round(h*.94))
 for y in bark_rows:
  if y>=h:break # canopy to the last row: no stem rows exist below it
  ix=np.where(a[y,:,3]>=220)[0];ix=ix[(ix>w*.3)&(ix<w*.7)]
  if len(ix)<2:continue
  center=int(np.median(ix));half=max(1,min(3,len(ix)//4));row=a[y,max(0,center-half):center+half+1];row=row[row[:,3]>=220]
  if len(row):bark.append(row[np.linspace(0,len(row)-1,16).astype(int)])
 if not bark and stem:raise NoStemData('No opaque stem data '+name)
 # A source with no opaque stem texels and stem=False builds no stem faces; tree.mtl still names trunk.tga, so write a 1x1 fully transparent placeholder (no colour is invented, no face uses it).
 if bark:ba=np.array(bark);ba[:,:,3]=255;Image.fromarray(ba).resize((64,len(ba)*4),Image.Resampling.LANCZOS).save(p/'trunk.tga')
 else:Image.new('RGBA',(1,1),(0,0,0,0)).save(p/'trunk.tga')
 # Alpha outline shapes the shell. Model horizontal extent exactly supplied source W/H.
 rows=[]
 for y in range(0,cut,max(2,h//64)):
  ix=np.where(a[y,:,3]>=128)[0]
  if len(ix):rows.append((y,ix.min(),ix.max()+1))
 if len(rows)<2:raise ValueError("Insufficient crown silhouette "+name)
 if adaptive and len(rows)>=5:
  # Smooth the silhouette so tiered branches become one shell, not a stack of discs; the source alpha still cuts the holes.
  lo_s=np.convolve(np.pad([r[1] for r in rows],2,mode='edge'),np.ones(5)/5,mode='valid');hi_s=np.convolve(np.pad([r[2] for r in rows],2,mode='edge'),np.ones(5)/5,mode='valid')
  rows=[(r[0],int(round(l)),int(round(hh))) for r,l,hh in zip(rows,lo_s,hi_s)]
 extent=w
 verts=[]
 for y,lo,hi in rows:
  cx=((lo+hi)/2-w/2)/h;r=(hi-lo)/extent*(w/h)/2
  for k in range(17):
   t=k*math.tau/16;verts.append((cx+r*math.cos(t),1-y/h,r*.8*math.sin(t),y,lo,hi))
 triangles=[]
 for j in range(len(rows)-1):
  for k in range(16):
   aa=j*17+k;bb=aa+17;triangles.extend([(aa,aa+1,bb),(aa+1,bb+1,bb)])
 out=[];faces=[]
 for f in triangles:
  q=np.array([verts[i][:3] for i in f]);nm=np.cross(q[1]-q[0],q[2]-q[0]);nm/=max(np.linalg.norm(nm),1e-12)
  faces.append(('crown',len(out)+1))
  for idx in f:
   x,y,z,sy,lo,hi=verts[idx];theta=(idx%17)/16
   out.append(((x,y,z),(theta,min(.999,max(.001,1-sy/cut))),nm))
 # Bake a continuous cross-projection atlas, not repeated image stripes.
 # Adjacent shell faces share exactly the same texture coordinates at seams.
 color=np.array(src.convert('RGBa').resize((w*4,h*4),Image.Resampling.LANCZOS).convert('RGBA')).astype(float)/255
 hh,ww=color.shape[:2];aw=1024;ah=cut*4;atlas=np.zeros((ah,aw,4),np.uint8)
 ts=np.arange(aw)/aw*math.tau;co=np.cos(ts);si=np.sin(ts)
 # Smooth, symmetric source image projection for front/back and side.
 wx=np.abs(si)**4;wz=np.abs(co)**4;den=np.maximum(wx+wz,1e-8)
 for iy in range(ah):
  sy=iy/4;ri=min(range(len(rows)),key=lambda k:abs(rows[k][0]-sy));_,lo,hi=rows[ri];cx=((lo+hi)/2-w/2)/h;r=(hi-lo)/h/2
  xx=cx+r*co;zz=r*si
  u0=np.clip((.5+xx*h/w)*ww,0,ww-1).astype(int);u1=np.clip((.5+zz*h/w)*ww,0,ww-1).astype(int);yy=min(hh-1,round(sy*hh/h));ca=color[yy,u0];cb=color[yy,u1]
  alpha=(ca[:,3]*wx+cb[:,3]*wz)/den;prem=(ca[:,:3]*ca[:,3,None]*wx[:,None]+cb[:,:3]*cb[:,3,None]*wz[:,None])/den[:,None];rgb=prem/np.maximum(alpha[:,None],1e-6)
  atlas[iy,:,:3]=np.clip(rgb*255,0,255).astype('uint8');atlas[iy,:,3]=np.clip(alpha*255,0,255).astype('uint8')
 Image.fromarray(atlas).save(p/'crown.tga')
 # Stem diameter measured from source opaque pixels, capped at 6% source width.
 stem_rows=[y for y in (range(min(cut+2,last_row-2),max(cut+3,last_row-1)) if adaptive else range(round(h*.82),round(h*.90))) if y<h] # rows past the image do not exist (canopy to the last row)
 stemwidth=min(w*.06,np.median([np.sum(a[y,:,3]>=220) for y in stem_rows])) if stem_rows else 0
 radius=max(.005,stemwidth/extent*(w/h)/2);stemtop=1-crown_bottom+.035
 for j in range(8 if stem else 0): # a dense shrub whose canopy reaches the ground has no stem in its source; a stub would be invented
  t=j*math.tau/8;v=(j+1)*math.tau/8;q=[(radius*math.cos(t),0,radius*math.sin(t)),(radius*math.cos(t)*.7,stemtop,radius*.7*math.sin(t)),(radius*math.cos(v)*.7,stemtop,radius*.7*math.sin(v)),(radius*math.cos(v),0,radius*math.sin(v))];uv=[(0,0),(0,1),(1,1),(1,0)];nm=(math.cos((t+v)/2),0,math.sin((t+v)/2))
  for f in [(0,1,2),(0,2,3)]:faces.append(('trunk',len(out)+1));out.extend([(q[i],uv[i],nm) for i in f])
 lines=[f'# opencmr2-source-canvas-1 {w} {h}','mtllib tree.mtl']
 for kind,j in [('v',0),('vt',1),('vn',2)]:lines += [kind+' '+' '.join(f'{v:.8f}' for v in q[j]) for q in out]
 last=None
 for mat,i in faces:
  if mat!=last:lines.append('usemtl '+mat);last=mat
  lines.append('f '+' '.join(f'{k}/{k}/{k}' for k in range(i,i+3)))
 (p/'source-vertices.json').write_text(json.dumps({'schema':'source-uv-per-obj-v-1','source_canvas_wh':[w,h],'vertices':[{'obj_v':i+1,'source_uv':[.5+float(q[0][0])*h/w,1-float(q[0][1])]} for i,q in enumerate(out)]},indent=2))
 (p/'recipe.json').write_text(json.dumps({'schema':'source-canvas-tree-1','source_canvas_wh':[w,h],'source_mapping':str(rel),'method':'4x premultiplied interpolation, no new detail; continuous cross-projection atlas','triangles':len(faces),'crown_pixels':[aw,ah],'stem_radius':float(radius)},indent=2)+'\n')
 (p/'tree.obj').write_text('\n'.join(lines)+'\n');(p/'tree.mtl').write_text('newmtl crown\nKd 1 1 1\nd 1\nmap_Kd crown.tga\nnewmtl trunk\nKd 1 1 1\nd 1\nmap_Kd trunk.tga\n')
 return str(rel/'tree.obj')

def crossed_tree(a,rel,with_stem=True):
 # Explicit opt-in cutout candidate, not a full3Dtree or private restored art.
 asset=tree(a,rel,stem=with_stem);p=root/rel;src=Image.open(a['path']).convert('RGBA');ar=np.array(src);h,w=ar.shape[:2]
 name=Path(a['texture']).stem;broad=name.startswith('tre_md')
 if broad:
  yy,xx=np.mgrid[:h,:w];ar[(yy>h*.72)&(abs(xx-w/2)<w*.08),3]=0
  ar[round(h*.94):,:,3]=0 # discard source ground grass, not tree foliage
 Image.fromarray(ar).convert('RGBa').resize((w*4,h*4),Image.Resampling.LANCZOS).convert('RGBA').save(p/'crossed.tga')
 old=(p/'tree.obj').read_text().splitlines();op=[];ot=[];on=[];stem=[];material=''
 for line in old:
  a=line.split()
  if not a:continue
  if a[0]=='v':op.append(a[1:])
  elif a[0]=='vt':ot.append(a[1:])
  elif a[0]=='vn':on.append(a[1:])
  elif a[0]=='usemtl':material=a[1]
  elif a[0]=='f' and material=='trunk':stem.append([list(map(int,x.split('/'))) for x in a[1:]])
 points=[];uv=[];normals=[[0,0,1]];faces=[]
 for k in range(4):
  theta=k*math.pi/4;co,si=math.cos(theta),math.sin(theta);start=len(points)
  for j in range(9):
   y=j/8;depth=.025*math.sin(j*1.7+k)*min(1,y*5)
   for x in [-w/h/2,w/h/2]:points.append([x*co-depth*si,y,x*si+depth*co]);uv.append([0 if x<0 else 1,y])
  for j in range(8):
   v=start+j*2;faces.extend([(v,v+1,v+2),(v+1,v+3,v+2)])
 lines=[f'# opencmr2-source-canvas-1 {w} {h}','mtllib tree.mtl'];ff=['usemtl crown']
 for face in faces:ff.append('f '+' '.join(f'{v+1}/{v+1}/1' for v in face))
 if broad:
  ff.append('usemtl trunk')
  for face in stem:
   ids=[]
   for v,t,n in face:
    points.append(list(map(float,op[v-1])));uv.append(list(map(float,ot[t-1])));normals.append(list(map(float,on[n-1])));ids.append(f'{len(points)}/{len(uv)}/{len(normals)}')
   ff.append('f '+' '.join(ids))
 lines+=['v '+' '.join(map(str,v)) for v in points]+['vt '+' '.join(map(str,v)) for v in uv]+['vn '+' '.join(map(str,v)) for v in normals]+ff
 (p/'tree.obj').write_text('\n'.join(lines)+'\n')
 (p/'tree.mtl').write_text('newmtl crown\nKd 1 1 1\nd 1\nmap_Kd crossed.tga\nnewmtl trunk\nKd 1 1 1\nd 1\nmap_Kd trunk.tga\n')
 (p/'source-vertices.json').write_text(json.dumps({'schema':'source-uv-per-obj-v-1','source_canvas_wh':[w,h],'vertices':[{'source_uv':[.5+v[0]*h/w,1-v[1]]}for v in points]},indent=2))
 metadata=json.loads((p/'recipe.json').read_text());metadata.update(method='4x premultiplied interpolation, no new detail; fixed source-projection cutouts',crown_pixels=[w*4,h*4],geometry='four fixed intersecting cutout planes, not full3D',triangles=len(faces)+(len(stem) if broad else 0),alpha_changes='broadleaf lower central stem and bottom6% removed from canopy; separate original stem' if broad else 'original source alpha interpolated')
 (p/'recipe.json').write_text(json.dumps(metadata,indent=2)+'\n');(p/'crown.tga').unlink();return asset

def articulated_spectator(src):
 # Exact manually authored demo recipe, only for hash-verified spc_mn9a.
 # This is four forearm poses on a 2D atlas, not a general skeleton.
 mask=Image.new('L',src.size);ImageDraw.Draw(mask).polygon([(64,13),(69,8),(96,4),(98,11),(75,16),(70,20)],fill=255)
 arm=src.copy();arm.putalpha(Image.composite(src.getchannel('A'),Image.new('L',src.size),mask))
 base=src.copy();base.putalpha(Image.composite(Image.new('L',src.size),src.getchannel('A'),mask))
 d=ImageDraw.Draw(base)
 d.polygon([(87,11),(88,8),(93,7),(98,8),(101,11),(101,14),(97,16),(92,15)],fill=(74,64,54,255))
 d.polygon([(89,11),(93,10),(99,11),(99,14),(94,16),(90,14)],fill=(129,111,94,255))
 d.polygon([(86,14),(86,11),(89,10),(91,12),(89,15)],fill=(58,50,43,255))
 poses=[]
 for angle in [0,10,20,10]:
  im=base.copy();im.alpha_composite(arm.rotate(gained(angle),resample=Image.Resampling.BICUBIC,center=(68,14)));poses.append(im)
 return poses

# Authored masks are normalized to the original128pxFranceatlases.
# High/low source variants are hash-gated by source-index before this function.
FOREGROUND_LIMBS = {'spc_mn15a': {'angles': [0, 8, 16, 8],
               'pivot': [9, 18],
               'plate': [],
               'polygon': [[6, 17], [8, 12], [18, 7], [23, 5], [26, 7], [24, 10], [16, 12], [11, 20]]},
 'spc_mn1a': {'angles': [0, 8, 16, 8],
              'pivot': [8, 18],
              'plate': [],
              'polygon': [[4, 16], [7, 12], [18, 7], [25, 4], [28, 7], [19, 11], [11, 19]]},
 'spe_05': {'angles': [0, -5, -10, -5],
            'pivot': [34, 38],
            'plate': [],
            'polygon': [[30, 38],
                        [36, 37],
                        [36, 48],
                        [34, 55],
                        [40, 58],
                        [40, 62],
                        [32, 63],
                        [28, 58],
                        [29, 48]]},
 'spe_10': {'angles': [0, 8, 16, 8],
            'pivot': [28, 19],
            'plate': [],
            'polygon': [[25, 17], [29, 12], [39, 6], [48, 4], [50, 9], [39, 11], [32, 20]]}}
# Explicit source-space masks. One person per group, not one whole-atlas warp.
EXPANDED_LIMBS = {'spc_mn11a': {'angles': [0, -7, -14, -7],
               'donor': [-7, 0],
               'pivot': [41, 59],
               'polygon': [[39, 57],
                           [44, 58],
                           [43, 64],
                           [39, 69],
                           [38, 75],
                           [34, 77],
                           [29, 74],
                           [32, 68],
                           [37, 64]]},
 'spc_mn13a': {'angles': [0, 7, 14, 7],
               'donor': [0, -10],
               'pivot': [31, 42],
               'polygon': [[7, 38],
                           [14, 39],
                           [23, 40],
                           [33, 40],
                           [34, 44],
                           [28, 46],
                           [16, 44],
                           [7, 42]]},
 'spc_mn7a': {'angles': [0, -9, -18, -9],
              'donor': [-2, -10],
              'pivot': [43, 47],
              'polygon': [[39, 45],
                          [46, 45],
                          [46, 50],
                          [40, 56],
                          [32, 60],
                          [27, 61],
                          [25, 58],
                          [30, 55],
                          [37, 51]]},
 'spe_06': {'angles': [0, -7, -14, -7],
            'donor': [-8, 0],
            'pivot': [96, 21],
            'polygon': [[87, 8],
                        [90, 6],
                        [94, 9],
                        [96, 16],
                        [99, 20],
                        [96, 24],
                        [92, 20],
                        [90, 13]]},
 'spe_07': {'angles': [0, 8, 16, 8],
            'donor': [0, 10],
            'pivot': [76, 19],
            'polygon': [[72, 17],
                        [74, 11],
                        [84, 8],
                        [94, 7],
                        [95, 11],
                        [87, 13],
                        [79, 17],
                        [77, 21]]},
 'spe_08': {'angles': [0, -8, -16, -8],
            'donor': [0, -9],
            'pivot': [59, 49],
            'polygon': [[39, 46],
                        [44, 47],
                        [51, 47],
                        [59, 47],
                        [61, 50],
                        [55, 53],
                        [46, 52],
                        [39, 50]]},
 'spe_09': {'angles': [0, -8, -16, -8],
            'donor': [0, 10],
            'pivot': [117, 39],
            'polygon': [[101, 33],
                        [105, 30],
                        [111, 33],
                        [117, 35],
                        [120, 39],
                        [117, 42],
                        [111, 38],
                        [105, 38]]}}
FINLAND_LIMBS = {'spc_ma02': {'angles': [0, -7, -14, -7],
              'pivot': [115, 43],
              'polygon': [[113, 41],
                          [118, 43],
                          [119, 54],
                          [118, 66],
                          [116, 78],
                          [111, 77],
                          [112, 67],
                          [113, 55]]},
 'spc_ma05': {'angles': [0, 7, 14, 7],
              'pivot': [13, 42],
              'polygon': [[11, 40],
                          [16, 41],
                          [16, 49],
                          [15, 58],
                          [15, 64],
                          [11, 66],
                          [8, 63],
                          [9, 55],
                          [10, 47]]},
 'spc_ma07': {'angles': [0, 7, 14, 7],
              'pivot': [12, 43],
              'polygon': [[8, 41],
                          [15, 41],
                          [15, 51],
                          [16, 61],
                          [15, 71],
                          [10, 72],
                          [8, 66],
                          [8, 55]]},
 'spc_ma08': {'angles': [0, -7, -14, -7],
              'pivot': [116, 44],
              'polygon': [[114, 42],
                          [119, 43],
                          [120, 51],
                          [119, 61],
                          [117, 68],
                          [113, 67],
                          [114, 59],
                          [114, 50]]},
 'spc_ma11': {'angles': [0, 7, 14, 7],
              'pivot': [11, 47],
              'polygon': [[9, 47],
                          [13, 45],
                          [18, 48],
                          [23, 49],
                          [25, 52],
                          [21, 55],
                          [15, 53],
                          [10, 51]]},
 'spc_ma12': {'angles': [0, -7, -14, -7],
              'pivot': [120, 37],
              'polygon': [[117, 35],
                          [122, 36],
                          [124, 43],
                          [124, 49],
                          [120, 53],
                          [116, 51],
                          [118, 46],
                          [117, 41]]}}
FOREGROUND_LIMBS.update(FINLAND_LIMBS)
FOREGROUND_LIMBS.update(EXPANDED_LIMBS)
ANIM_GAIN=1.0  # --animation-gain: scales the spectators' sway distance and arm swing angle; 1.0 is the reviewed original

def gained(angle):
 return max(-110.0,min(110.0,angle*ANIM_GAIN))

def foreground_limb_poses(src,name):
 original_size=src.size
 if src.size!=(128,128):src=src.resize((128,128),Image.Resampling.NEAREST)
 if name=='spc_mn9a':poses=articulated_spectator(src)
 else:
  r=FOREGROUND_LIMBS[name];mask=Image.new('L',src.size);ImageDraw.Draw(mask).polygon([tuple(p) for p in r['polygon']],fill=255)
  arm=src.copy();arm.putalpha(Image.composite(src.getchannel('A'),Image.new('L',src.size),mask))
  base=src.copy();base.putalpha(Image.composite(Image.new('L',src.size),src.getchannel('A'),mask))
  if name in EXPANDED_LIMBS:
   dx,dy=r['donor']
   for y in range(128):
    for x in range(128):
     if mask.getpixel((x,y)) and 0<=x+dx<128 and 0<=y+dy<128:
      px=src.getpixel((x+dx,y+dy))
      if px[3] and not mask.getpixel((x+dx,y+dy)):base.putpixel((x,y),px)
   if name=='spe_07':
    base=src.copy();base.putalpha(Image.composite(Image.new('L',src.size),src.getchannel('A'),mask))
    ImageDraw.Draw(base).polygon([(87,9),(90,7),(94,8),(96,11),(95,14),(91,13)],fill=src.getpixel((93,14)))
  if name=='spe_05':
   # Continue the occluded purple-shirt neighbour behind the moving arm.
   for y in range(38,64):
    for x in range(28,41):
     if mask.getpixel((x,y)):base.putpixel((x,y),src.getpixel((x-7,y)))
  plates={
   'spe_10':([(46,8),(48,6),(51,7),(51,11),(49,13),(47,11)],(156,128,111,255)),
   'spc_mn1a':([(23,8),(24,6),(27,6),(29,8),(29,11),(26,12)],(151,125,108,255)),
   'spc_mn15a':([(21,8),(23,7),(26,8),(28,10),(26,12),(22,11)],(124,104,86,255))}
  if name in plates:
   points,color=plates[name];ImageDraw.Draw(base).polygon(points,fill=color)
  poses=[]
  for angle in r['angles']:
   im=base.copy();im.alpha_composite(arm.rotate(gained(angle),resample=Image.Resampling.BICUBIC,center=tuple(r['pivot'])));poses.append(im)
 return [im.resize((original_size[0]*2,original_size[1]*2),Image.Resampling.NEAREST) for im in poses]

def spectator(a,rel):
 p=root/rel;p.mkdir(parents=True,exist_ok=True);src=Image.open(a['path']).convert('RGBA');frames=[]
 name=Path(a.get('texture','')).stem
 if a.get('archive') in ('frahit.bfl','fralot.bfl','finhit.bfl','finlot.bfl') and (name in FOREGROUND_LIMBS or name=='spc_mn9a'):
  for i,pose in enumerate(foreground_limb_poses(src,name)):
   path=p/f'frame-{i:02}.tga';pose.save(path,compression=None);frames.append(str(rel/path.name))
  return frames
 src=src.resize((src.width*2,src.height*2),Image.Resampling.NEAREST);ar=np.array(src);h,w=ar.shape[:2]
 # Subtle upper-body sway: original cutout remapped, lower 30% and feet fixed. No invented limbs.
 yy,xx=np.mgrid[:h,:w]
 for i in range(12):
  weight=np.clip((.70-yy/h)/.70,0,1)**2;dx=np.rint(math.sin(i*math.tau/12)*max(1,w*.009)*ANIM_GAIN*weight).astype(int);sx=xx-dx;ok=(sx>=0)&(sx<w)
  sy=yy
  if ANIM_GAIN>1.0:
   # Above gain 1 the sway-only people also bob: the upper body rises and settles twice per loop (lower 30 percent and feet stay put), so a pure shear is not the only motion.
   lift=np.rint(abs(math.sin(i*math.tau/12*2))*h*.012*(ANIM_GAIN-1.0)*np.clip((.70-yy/h)/.70,0,1)).astype(int);sy=yy+lift;ok&=(sy<h)
  out=np.zeros_like(ar);out[ok]=ar[sy[ok],sx[ok]];path=p/f'frame-{i:02}.tga';Image.fromarray(out).save(path,compression=None);frames.append(str(rel/path.name))
 return frames

def read_archive(path, name, index):
 """Read and verify supported DDS entries without writing any output."""
 import gzip, struct, hashlib
 # Bound decompression before parsing untrusted local archives.
 with gzip.open(path, 'rb') as stream:
  blob = stream.read(256 * 1024 * 1024 + 1)
 if len(blob) > 256 * 1024 * 1024 or len(blob) < 12 or blob[:4] != b'CMPR':
  raise ValueError('Unsupported BFL header or size: ' + name)
 b = blob[8:]
 directory = struct.unpack_from('<I', b, len(b)-4)[0]
 if directory > len(b)-4:
  raise ValueError('BFL directory outside file: ' + name)
 pos = directory
 data = {}
 seen = set()
 while pos < len(b)-4:
  if pos+12 > len(b)-4:
   raise ValueError('Truncated BFL directory')
  size, offset, n = struct.unpack_from('<III', b, pos)
  pos += 12
  padded = (n+3)//4*4
  if n == 0 or n > 255 or pos+padded > len(b)-4:
   raise ValueError('Invalid BFL entry name')
  leaf = b[pos:pos+n].decode('ascii')
  pos += padded
  if '/' in leaf or chr(92) in leaf or '..' in leaf or offset+size > directory or leaf in seen:
   raise ValueError('Unsafe or duplicate BFL entry')
  seen.add(leaf)
  key = name+'/'+leaf
  if key in index:
   raw = b[offset:offset+size]
   if hashlib.sha256(raw).hexdigest() != index[key]['sha256']:
    raise ValueError('Unsupported source texture hash: '+key+'; no output written')
   with Image.open(__import__('io').BytesIO(raw)) as image:
    if image.size != (index[key]['width'], index[key]['height']):
     raise ValueError('Unsupported source dimensions: '+key)
   data[key] = raw
 return data

def main():
 import argparse,tempfile,os
 ap=argparse.ArgumentParser(description='Local procedural asset patcher: requires verified original game files; contains no artwork. Never alters inputs.')
 ap.add_argument('--input-dir',type=Path,required=True)
 ap.add_argument('--output-dir',type=Path,required=True)
 ap.add_argument('--tree-geometry',choices=['shell','crossed'],default='shell',help='Experimental fixed cutout alternative; neither mode has native visual acceptance')
 ap.add_argument('--animation-gain',type=float,default=1.0,help='Experimental: multiply spectator sway distance and arm swing angle (1.0 to 4.0, default 1.0 = unchanged); not native-verified')
 ap.add_argument('--all-trees',action='store_true',help='Experimental: also convert every other individual tree texture (adaptive crown/stem; not native-verified). Default keeps the 3 reviewed mappings.')
 ap.add_argument('--check-only',action='store_true',help='Verify originals without creating output')
 args=ap.parse_args()
 if not 1.0<=args.animation_gain<=4.0:ap.error("--animation-gain expects 1.0..4.0")
 global ANIM_GAIN;ANIM_GAIN=args.animation_gain
 global root
 root=args.output_dir.resolve();inp=args.input_dir.resolve()
 if not inp.is_dir():ap.error('Input must be an existing directory.')
 if root==inp or inp in root.parents:ap.error('Choose output outside original game input directory.')
 if root.exists() and (not root.is_dir() or any(root.iterdir())):ap.error('Output must be new or empty; will not replace existing files.')
 index=json.loads(Path(__file__).with_name('source-index.json').read_text(encoding='utf-8'))
 archives=sorted({key.split('/')[0] for key in index});paths={}
 # Do not traverse symlink directories or read linked files outside the owned input.
 for directory,dirs,files in os.walk(inp,followlinks=False):
  dirs[:]=[d for d in dirs if not (Path(directory)/d).is_symlink()]
  for leaf in files:
   p=Path(directory)/leaf
   if leaf.lower() in archives and not p.is_symlink():paths.setdefault(leaf.lower(),[]).append(p)
 for name in archives:
  if len(paths.get(name,[]))!=1:ap.error(f'Need exactly one {name}; found {len(paths.get(name,[]))}')
 data={}
 for name in archives:data.update(read_archive(paths[name][0],name,index))
 if set(data)!=set(index):raise ValueError('Missing required DDS entries; no output written')
 if args.check_only:
  print(f'Verified {len(data)} source textures. No output written.');return
 # Publish the complete local output only after successful generation.
 root.parent.mkdir(parents=True,exist_ok=True)
 destination=root
 with tempfile.TemporaryDirectory(prefix='.cmr2-patch-',dir=root.parent) as staging:
  root=Path(staging)/'pack';root.mkdir()
  rows=['kind\tsource_archive\tsource_texture\tasset\tframes\tticks'];coverage=[];identities=[];wind_identities=[]
  with tempfile.TemporaryDirectory(prefix='cmr2-source-') as tmp:
   for key,rec in index.items():
    archive,leaf=key.split('/');a={**rec,'archive':archive,'texture':leaf};rel=Path(Path(archive).stem)/Path(leaf).stem
    source=Path(tmp)/archive/leaf;source.parent.mkdir(exist_ok=True);source.write_bytes(data[key]);a['path']=str(source)
    if rec.get('category')=='individual-tree' and ((archive,Path(leaf).stem) in TREE_CANDIDATES or (args.tree_geometry=='crossed' and (archive,Path(leaf).stem) in CROSSED_CANDIDATES)):
     asset=(crossed_tree if args.tree_geometry=='crossed' else tree)(a,Path('trees')/rel);rows.append('\t'.join(['tree',archive,leaf,asset,'','1']));status='source-fitted-interpolated'
    elif rec.get('category') in ('individual-tree','individual-bush') and args.all_trees:
     # Every other individual tree: crown/stem split from its own alpha. Sparse crowns use crossed planes, dense ones the smoothed shell.
     alpha=np.array(Image.open(a['path']).convert('RGBA'))[:,:,3];bush=rec.get('category')=='individual-bush'
     # Bushes: the source decides. A canopy that reaches the bottom rows has no stem, so none is built. The Finland bsh_01/02 textures are
     # horizontal leaf strips on a palm-like trunk: a closed shell turned them into a perforated cage in the contact sheet, so they use the crossed planes that keep the strips whole.
     leaf_strips=bush and archive in ('finhit.bfl','finlot.bfl') and Path(leaf).stem in ('bsh_01','bsh_02')
     # Private all-source preview rejected closed glossy blobs for these narrow birches.
     source_cutout=Path(leaf).stem in ('tre_wb1','tre_gr1')
     sparse=args.tree_geometry=='crossed' or bush or source_cutout or is_sparse(alpha)
     no_stem=bush and crown_profile(alpha)[0]>=.9
     def build(no_stem):return crossed_tree(a,Path('trees')/rel,with_stem=not no_stem) if sparse else tree(a,Path('trees')/rel,adaptive=True,stem=not no_stem)
     try:asset=build(no_stem)
     except NoStemData: # --all-trees only: the source has no opaque stem texels (e.g. Greece tre_40), so build the crown without inventing a stem instead of aborting the whole pack
      no_stem=True;asset=build(True)
     rows.append('\t'.join(['tree',archive,leaf,asset,'','1']));status=('adaptive-crossed' if sparse else 'adaptive-shell')+('-no-stem-data' if no_stem and not bush else '')
    elif rec.get('category')=='spectator':
     frames=spectator(a,Path('spectators')/rel);rows.append('\t'.join(['spectator',archive,leaf,frames[0],','.join(frames),'6' if len(frames)==4 else '5']));status='articulated-forearm' if len(frames)==4 else 'upper-body-sway'
    else:status='retained-original'
    if rec.get('category')=='individual-tree':wind_identities.append('\t'.join([key,str(len(data[key])),rec['sha256']]))
    if status!='retained-original':identities.append('\t'.join([key,str(len(data[key])),rec['sha256']]))
    coverage.append({k:v for k,v in a.items() if k!='path'}|{'status':status})
  (root/'tree-wind-identities.tsv').write_text('\n'.join(wind_identities)+'\n')
  (root/'source-identities.tsv').write_text('\n'.join(identities)+'\n')
  (root/'manifest.tsv').write_text('\n'.join(rows)+'\n',encoding='utf-8')
  (root/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n',encoding='utf-8')
  if destination.exists():destination.rmdir() # fails if another process populated it
  root.rename(destination)
 root=destination
 print(f'Created {len(rows)-1} sidecar entries in {root}. Originals unchanged.')
if __name__=='__main__':main()
