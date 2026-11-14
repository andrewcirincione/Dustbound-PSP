#!/usr/bin/env python3
"""Calibrate generated foundations to the 2:1 grid, then compile compact sprites."""
from pathlib import Path
from PIL import Image
import json,sys,hashlib
from pack_structure_art import fit,shadow,pack_shadow
from pack_settlement_art import pack
root=Path(__file__).resolve().parents[1];art=root/'assets/art/exploration';packed=art/'packed';packed.mkdir(exist_ok=True)
names=['relay-citadel','turbine-hall','caravan-refuge','sealed-archive','scrapper-yard','pumpworks',*[f'{bot}-{frame}' for bot in ['sentinel','crawler'] for frame in range(4)]]
specs=[((.01,.79),(.36,.99),(.99,.87),.77,1,152,135),((.01,.91),(.84,.995),(.995,.87),.925,1,176,138),((.01,.99),(.80,.995),(.995,.54),.68,0,166,130),((.01,.97),(.44,.995),(.99,.95),.84,1,165,162),((.01,.90),(.82,.995),(.99,.77),.73,0,174,132),((.01,.985),(.87,.995),(.99,.72),.80,0,156,122)]
meta=json.loads((art/'models.json').read_text()) if (art/'models.json').exists() else []
if '--rebuild-source' in sys.argv:
 meta=[]
 frames={}
 for bot in ['sentinel','crawler']:
  sheet=Image.open(art/'originals'/(bot+'-walk.png')).convert('RGBA');tiles=[]
  for k in range(4):
   im=sheet.crop(((k%2)*sheet.width//2,(k//2)*sheet.height//2,(k%2+1)*sheet.width//2,(k//2+1)*sheet.height//2));im.putalpha(im.getchannel('A').point(lambda a:255 if a>=180 else 0));im=im.crop(im.getbbox());tiles.append(im)
  scale=(64 if bot=='sentinel' else 48)/tiles[0].height
  feet=[]
  for im in tiles:
   ap=im.getchannel('A');ys=range(int(im.height*.87),im.height);xs=[x for y in ys for x in range(im.width) if ap.getpixel((x,y))>=128];feet.append((sum(xs)/len(xs),im.height-1))
  left=max(f[0]*scale for f in feet);right=max((im.width-f[0])*scale for im,f in zip(tiles,feet));ww=round(2*max(left,right)+8);hh=round(max(im.height for im in tiles)*scale)+8;anchor=(ww//2,hh-3)
  for k,(im,foot) in enumerate(zip(tiles,feet)):
   resized=im.resize((round(im.width*scale),round(im.height*scale)),Image.Resampling.LANCZOS);frame=Image.new('RGBA',(ww,hh));frame.paste(resized,(round(anchor[0]-foot[0]*scale),round(anchor[1]-foot[1]*scale)));frames[bot+'-'+str(k)]=(frame,anchor)
 for i,n in enumerate(names):
  source=art/'originals'/((n.split('-')[0]+'-walk.png') if i>=6 else n+'.png')
  im=Image.open(source).convert('RGBA');im.putalpha(im.getchannel('A').point(lambda a:255 if a>=180 else 0));im=im.crop(im.getbbox())
  if i<6:im,anchor,front,model=fit(im,specs[i])
  else:
   im,anchor=frames[n];model=None
  im.save(packed/(n+'.png'));meta.append({'name':n,'anchor':anchor,'model':model,'generated_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'spec':specs[i] if i<6 else None})
 (art/'models.json').write_text(json.dumps(meta,indent=2)+'\n')
assert len(meta)==14
code=['/* Compiled generated, calibrated building and robot art. */\n'];entries=[];shadows=[];models=[]
for i,z in enumerate(meta):
 im=Image.open(packed/(z['name']+'.png')).convert('RGBA');assert im.width*im.height<65536
 src,entry=pack(im,'explore_'+str(i),z['anchor'][1],z['anchor'][0]);code.append(src);entries.append(entry)
 if i<6:
  m=z['model'];models.append('{'+','.join(('%.6ff'%v) if j<4 else str(int(v)) for j,v in enumerate(m))+'}')
  # The packed forward foundation corner is inferable from its 2:1 dimensions.
  corner=(z['anchor'][0]+22*(m[0]-m[1]),z['anchor'][1]+11*(m[0]+m[1]))
  sh,sa=shadow(im,z['anchor'],corner);sc,se=pack_shadow(sh,'explore_shadow_'+str(i),sa);code.append(sc);shadows.append(se)
code.append('static const ArtSprite expeditionSprites[]={\n'+',\n'.join(entries)+'\n};\nstatic const ArtShadow expeditionShadows[]={\n'+',\n'.join(shadows)+'\n};\n#define EXPEDITION_ART_READY 14\n')
(root/'src/expedition_art.h').write_text(''.join(code));(root/'src/expedition_models.h').write_text('/* Generated calibrated foundation and doorway geometry. */\nstatic const BuildingModel expeditionModels[]={\n'+',\n'.join(models)+'\n};\n')
print('Packed six calibrated building models and two robot enemies.')
