"""Normal PSP controller input against the final EBOOT; prepared ordinary saves, read-only QA."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,hashlib,math
import websockets
from emulator_v060 import Play,ROOT
NAMES=['scavenger-camp','refugee-camp','hunters-camp','smugglers-den','traders-rest','road-patrol','raiding-band','beast-pack','pilgrimage','wounded-travelers','battlefield','marching-army','bandit-ambush','salvage-crew','prisoner-convoy','wandering-duelists']
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v120-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 shutil.copy2(ROOT/'build/v120/encounter-00.sav',run/'DUST_1.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([sys.argv[1],'--graphics=software','--debugger=9251','--timeout=1500','-r',str(run),str(run/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;e.events[0]['version']='0.12.0';await asyncio.sleep(.8);await e.call('cpu.resume')
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
    if fixture:shutil.copy2(ROOT/'build/v120'/fixture,run/f'DUST_{slot}.sav')
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
   await e.shot('v120-psp-title',13)
   await key('cross');await expect('screen',15);await choose(9);await key('cross');await expect('screen',0)
   stats=await e.read('population','8i');assert stats[0]==80 and stats[2]>250 and stats[3]<stats[2]//10
   await e.shot('v120-psp-fresh-expedition',0);e.events.append({'fresh_start_population':list(stats),'normal_character_creation':True})
   await save();await load(6);assert (await e.read('population','8i'))[2:4]==stats[2:4]
   for t,name in enumerate(NAMES):
    await load(fixture=f'encounter-{t:02d}.sav',initial=False)
    near=await e.read('pop_near','12i');assert near[1]==t and near[5]==1,(t,near)
    await e.shot('v120-psp-'+name,0)
    e.events.append({'encounter':name,'members':near[6],'beds':near[4],'serial':near[3]})
    if t in [0,4,9,10,11,14]:
     await key('cross');await e.shot('v120-psp-actions-'+name,35);await key('circle')
    if t==11:
     assert 12<=near[6]<=16
     before=await e.read('pop_position','ff');await e.waitframes(100);after=await e.read('pop_position','ff')
     assert math.dist(before,after)>.1,(before,after)
     e.events.append({'army_moves_without_player_order':True,'distance':math.dist(before,after)})
   await load(fixture='encounter-04.sav')
   health=(await e.read('health','6f'))[2];fatigue=(await e.read('fatigue','f'))[0]
   await action('Rest in camp bed','v120-psp-free-bed-action');await expect('task',3)
   await e.waitframes(100);assert (await e.read('health','6f'))[2]>health and (await e.read('fatigue','f'))[0]<fatigue
   await e.shot('v120-psp-resting-in-camp-bed',0)
   await menu(1,2);await e.shot('v120-psp-bed-recovered-health',3);await key('start')
   if (await e.read('screen'))[0]==11:await key('circle')
   x,y=await e.read('pop_position','ff');await move(x,y)
   money=(await e.read('money'))[0];rations=await count(1);stock=(await e.read('pop_near','12i'))[7]
   await action('Buy Trail ration','v120-psp-camp-barter');await expect('money',money-40);assert await count(1)==rations+1
   assert (await e.read('pop_near','12i'))[7]==stock-1
   meat=await count(9);rations=await count(1)
   await action('Cook at fire','v120-psp-camp-cooking');assert await count(9)==meat-2 and await count(1)==rations+1
   stock=(await e.read('pop_near','12i'))[7];money=(await e.read('money'))[0];await save();await load(6)
   assert (await e.read('pop_near','12i'))[7]==stock;await expect('money',money)
   await e.shot('v120-psp-camp-state-reloaded',0)
   await load(fixture='random-prosthetic.sav')
   await action('Loot ');await expect('screen',24)
   fitted=await e.read('loot_fitted','4i');socket=next(i for i,it in enumerate(fitted) if it);it=fitted[socket];before=await count(it)
   await choose(30+socket);await e.shot('v120-psp-random-wanderer-limb',24);await key('cross')
   assert await count(it)==before+1 and (await e.read('loot_fitted','4i'))[socket]==0
   await e.shot('v120-psp-random-limb-taken',24)
   await key('cross');assert await count(it)==before+1
   await save();await load(6);assert await count(it)==before+1
   await action('Loot ');await expect('screen',24);assert (await e.read('loot_fitted','4i'))[socket]==0
   await e.shot('v120-psp-random-loot-stays-gone',24)
   e.events.append({'completed':True,'prepared_encounter_types':16,'bed_health_and_fatigue_recovery':True,'finite_barter_and_cooking':True,'camp_stock_save_reload':True,'naturally_rolled_fitted_limb_looted_once_and_saved':True,'read_only_qa':True,'eboot_sha256':hashlib.sha256((run/'EBOOT.PBP').read_bytes()).hexdigest()})
   (ROOT/'evidence/v120-controller-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('PSP 0.12.0 CONTROLLER PASS',run,flush=True)
 except Exception:
  if 'e' in locals():
   try:await e.shot('v120-psp-failure')
   except Exception:pass
  raise
 finally:proc.terminate();proc.wait(timeout=10);log.close()
if __name__=='__main__':asyncio.run(main())
