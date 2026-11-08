"""Normal PSP controller input against the final EBOOT; prepared ordinary saves, read-only QA."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,hashlib,math
import websockets
from emulator_v060 import Play,ROOT
NAMES=['scavenger-camp','refugee-camp','hunters-camp','smugglers-den','traders-rest','road-patrol','raiding-band','beast-pack','pilgrimage','wounded-travelers','battlefield','marching-army','bandit-ambush','salvage-crew','prisoner-convoy','wandering-duelists']
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v130-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 shutil.copy2(ROOT/'build/v130/camp-0.sav',run/'DUST_1.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([sys.argv[1],'--graphics=software','--debugger=9251','--timeout=2400','-r',str(run),str(run/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;e.events[0]['version']='0.13.0';await asyncio.sleep(.8);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   async def key(k,n=1):
    for _ in range(n):await e.press(k,5);await e.waitframes(12)
   async def expect(field,v):assert (await e.read(field))[0]==v,(field,await e.read(field),v)
   async def choose(n):
    for _ in range(70):
     current=(await e.read('selection'))[0]
     if current==n:return
     await key('down' if current<n else 'up')
    raise AssertionError(('selection',await e.read('selection'),n))
   async def menu(tab,row):
    if (await e.read('screen'))[0]!=11:await key('start')
    if (await e.read('screen'))[0]==0:await key('start')
    await expect('screen',11)
    while (await e.read('menutab'))[0]!=tab:await key('rtrigger')
    await choose(row);await key('cross')
   async def load(slot=1,fixture=None,initial=False):
    if fixture:shutil.copy2(ROOT/'build/v130'/fixture,run/f'DUST_{slot}.sav')
    if initial:await choose(2);await key('cross')
    else:await menu(0,2)
    await expect('screen',22);await choose(slot);await key('cross');await expect('screen',14);await key('cross')
    if (await e.read('screen'))[0]==14:await key('cross')
    await expect('screen',0)
   async def rows():
    raw=bytes.fromhex(json.loads((run/'DUST_STATE.json').read_text())['actionlabels_value'])
    return [raw[i:i+80].split(b'\0')[0].decode() for i in range(0,len(raw),80)]
   async def action(fragment,capture=None):
    await key('cross');await expect('screen',35);labels=await rows()
    index=next(i for i,r in enumerate(labels) if fragment.lower() in r.lower())
    await choose(index)
    if capture:await e.shot(capture,35)
    await key('cross')
   async def count(it):
    pack=await e.read('pack','48i');return sum(pack[i+1] for i in range(0,48,2) if pack[i]==it)
   async def move(x,y):
    for _ in range(250):
     px,py=await e.read('position','ff');dx=x-px;dy=y-py
     if math.hypot(dx,dy)<.3:return
     sx,sy=dx-dy,dx+dy;scale=max(abs(sx),abs(sy),1)
     await e.call('input.analog.send',x=.8*sx/scale,y=-.8*sy/scale)
     await e.waitframes(4);await e.call('input.analog.send',x=0,y=0);await e.waitframes(2)
    raise AssertionError(('analog route blocked',x,y,await e.read('position','ff')))
   async def save(slot=6):
    await menu(0,1);await expect('screen',22);await choose(slot);await key('cross');await expect('screen',14);await key('cross')
   await e.shot('v130-psp-title',13)
   await key('cross');await expect('screen',15);await choose(9);await key('cross');await expect('screen',0)
   await e.shot('v130-psp-fresh-world',0)
   stats=await e.read('population','8i');assert stats[0]==80 and stats[3]<stats[2]//10
   # An ordinary fresh-start journey: controller movement with the world running.
   start_time=(await e.read('time','f'))[0];start_frame=(await e.read('framecount'))[0]
   import time
   wall=time.monotonic();journey=[]
   points=[(26,34),(26,37),(32,37),(40,37),(50,37),(60,37)]
   initial_position=await e.read('position','ff')
   for leg,(x,y) in enumerate(points):
    await move(x,y);await e.waitframes(90)
    journey.append({'position':list(await e.read('position','ff')),'time':(await e.read('time','f'))[0],'population':list(await e.read('population','8i'))})
    if leg in [1,3,5]:await e.shot(f'v130-psp-road-journey-{leg}',0)
   assert math.dist(initial_position,await e.read('position','ff'))>30
   elapsed=time.monotonic()-wall;frames=(await e.read('framecount'))[0]-start_frame
   e.events.append({'fresh_controller_journey':journey,'simulated_seconds':(await e.read('time','f'))[0]-start_time,'host_wall_seconds':elapsed,'observed_frame_rate':frames/elapsed,'physical_psp_benchmark':False})
   await save();await load(6);await e.shot('v130-psp-journey-reloaded',0)
   for t in range(5):
    await load(fixture=f'camp-{t}.sav');await e.waitframes(30)
    await e.shot(f'v130-psp-camp-{t}',0)
   health=(await e.read('health','6f'))[2];fatigue=(await e.read('fatigue','f'))[0]
   await action('Rest in camp bed','v130-psp-camp-bed-action');await expect('task',3)
   await e.waitframes(100);assert (await e.read('health','6f'))[2]>health and (await e.read('fatigue','f'))[0]<fatigue
   await e.shot('v130-psp-camp-bed-rest',0)
   await load(fixture='rescue.sav');await e.waitframes(100)
   assert (await e.read('life','8i'))[1]>0
   carry=await e.read('life_carry','32i');assert any(x>=0 for x in carry),carry
   x,y=await e.read('pop_position','ff');await move(x+1,y+2);await e.shot('v130-psp-companion-rescue',0);await save();await load(6)
   assert any(x>=0 for x in await e.read('life_carry','32i'))
   await e.shot('v130-psp-rescue-reloaded',0)
   await load(fixture='ambush-ready.sav');await e.waitframes(30)
   assert (await e.read('life_near','12i'))[5]==0
   await e.shot('v130-psp-ambush-hidden',0)
   await move(380,385);await e.waitframes(35)
   assert (await e.read('life_near','12i'))[5]>0
   await e.shot('v130-psp-ambush-sprung',0)
   await load(fixture='council.sav');await action('Council','v130-psp-council-action');await expect('screen',41)
   await e.shot('v130-psp-council-open',41)
   money=(await e.read('money'))[0];before=await e.read('council','10i');rations=await count(1)
   await choose(1);await key('cross');await expect('money',money-200)
   assert (await e.read('council','10i'))[3]==before[3]+30
   await key('cross');await expect('money',money-200)
   await choose(2);await key('cross');assert await count(1)==rations-4
   await key('cross');assert await count(1)==rations-4
   await e.shot('v130-psp-council-pledged',41)
   await save();await load(6);await action('Council');await expect('screen',41)
   after=await e.read('council','10i');assert after[4]==1 and after[5]==1
   await e.shot('v130-psp-council-reloaded',41)
   await load(fixture='council-near-resolution.sav');await e.waitframes(130)
   await action('Council');assert (await e.read('council','10i'))[1]==2
   await e.shot('v130-psp-new-government',41)
   await load(fixture='activity-rich.sav');await menu(2,8);await expect('screen',40)
   await e.shot('v130-psp-world-activity',40);await choose(9);await e.shot('v130-psp-world-activity-scroll',40)
   await load(fixture='limb-choices.sav');await menu(1,2);await key('rtrigger');await expect('screen',36)
   await choose(0);await key('cross');await expect('screen',37)
   for grade in [2,3,4]:
    await choose(grade);await e.shot(f'v130-psp-arm-grade-{grade}',37)
   await choose(2);parts=await count(33);await key('cross');await expect('screen',36)
   assert (await e.read('fitted','4i'))[0]==2370 and await count(33)==parts-2
   await choose(2);await key('cross');await expect('screen',37)
   for grade in [2,3,4]:
    await choose(grade);await e.shot(f'v130-psp-leg-grade-{grade}',37)
   await choose(3);await key('cross');await expect('screen',36)
   assert (await e.read('fitted','4i'))[2]==2373
   await e.shot('v130-psp-specialized-limbs-fitted',36)
   await save();await load(6);assert (await e.read('fitted','4i'))[0]==2370 and (await e.read('fitted','4i'))[2]==2373
   e.events.append({'completed':True,'camp_variants':5,'bed_recovery':True,'companion_carry_saved':True,'ambush_hidden_then_triggered':True,'finite_pledge_and_relief_saved':True,'succession_resolved':True,'activity_scroll':True,'specialized_limb_previews_fitting_saved':True,'read_only_qa':True,'eboot_sha256':hashlib.sha256((run/'EBOOT.PBP').read_bytes()).hexdigest()})
   (ROOT/'evidence/v130-controller-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('PSP 0.13.0 CONTROLLER PASS',run,flush=True)
 except Exception:
  if 'e' in locals():
   try:await e.shot('v130-psp-failure')
   except Exception:pass
  raise
 finally:proc.terminate();proc.wait(timeout=10);log.close()
if __name__=='__main__':asyncio.run(main())
